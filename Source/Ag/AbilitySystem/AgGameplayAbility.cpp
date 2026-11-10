// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystem/AgGameplayEffectContext.h"
#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/AgTargeting.h"
#include "AbilitySystem/Tasks/AgAbilityTask_PlayMontage.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilitySystemGlobals.h"
#include "Character/AgCharacterBase.h"
#include "Core/AgAssetManager.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgAttackData.h"
#include "Data/AgEffectData.h"
#include "Data/AgTaskParams.h"
#include "Ag.h"

namespace AgGameplayAbilityPrivate
{
	FGameplayEffectQuery CooldownQuery(const UObject* Source)
	{
		FGameplayEffectQuery Query =
			FGameplayEffectQuery::MakeQuery_MatchAnyEffectTags(FGameplayTagContainer(AgGameplayTags::Effect_Cooldown));
		Query.EffectSource = Source;
		return Query;
	}
}

UAgGameplayAbility::UAgGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
	// 엔진은 재발동 여부를 클래스 기본 객체의 값으로 정하므로 모두 허용하고, 행동마다의 허용(bAllowRetrigger)은
	// CanActivateAbility가 봅니다.
	bRetriggerInstancedAbility = true;
}

void UAgGameplayAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	AAgCharacterBase* Character = ActorInfo ? Cast<AAgCharacterBase>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (Character && !HasAnyFlags(RF_ClassDefaultObject))
	{
		AbilityId = Character->FindAbilityId(Spec.Handle);
		// 엔진이 이 인스턴스의 스펙 핸들을 넣기(Super) 전이라 캐릭터에서 바로 찾습니다.
		if (const FAgAbilityData* Data = Character->FindAbility(Spec.Handle))
		{
			Definition = *Data;
			ActivationRequiredTags = Data->ActivationRequiredTags;
			ActivationBlockedTags = Data->ActivationBlockedTags;
			ActivationOwnedTags = Data->ActivationOwnedTags;
			BlockAbilitiesWithTag = Data->BlockAbilitiesWithTag;
			CancelAbilitiesWithTag = Data->CancelAbilitiesWithTag;
		}
	}
	Super::OnGiveAbility(ActorInfo, Spec);
}

bool UAgGameplayAbility::CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	// 부여한 인스턴스에서만 답합니다. 발동 조건은 부여할 때 인스턴스에 넣으므로 클래스 기본 객체는 정의를 찾지 못합니다.
	const FAgAbilityData* Data = FindDefinition();
	if (!Data || !Definition.HasSameGrantSettings(*Data))
	{
		UE_LOG(LogAg, Warning,
			TEXT("Ability %s: grant settings changed or unavailable; recreate the character before activation"),
			*AbilityId.ToString());
		return false;
	}
	const FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec();
	if (!Spec || (Spec->IsActive() && !Data->bAllowRetrigger))
	{
		return false;
	}
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	return !Data->Targeting || !Data->Targeting->bRequired || FindTargetingTarget(*Data);
}

bool UAgGameplayAbility::DoesAbilitySatisfyTagRequirements(const UAbilitySystemComponent& ASC, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	// 엔진의 차단 검사는 클래스의 어빌리티 태그를 보므로, 스펙에 둔 행동 태그의 차단도 확인합니다.
	return !ASC.AreAbilityTagsBlocked(Definition.AbilityTags) &&
	       Super::DoesAbilitySatisfyTagRequirements(ASC, SourceTags, TargetTags, OptionalRelevantTags);
}

bool UAgGameplayAbility::CheckCost(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	// 활성 스펙의 재발동 사전 검사도 여기로 들어오므로 새 요청의 현재 정의를 확인합니다.
	const FAgAbilityData* Data = FindDefinition();
	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (Data && ASC && ASC->GetNumericAttribute(UAgAttributeSet::GetMPAttribute()) >= Data->Cost.MP &&
		ASC->GetNumericAttribute(UAgAttributeSet::GetUPAttribute()) >= Data->Cost.UP &&
		!Data->ActivationAttributes.ContainsByPredicate([ASC](const FAgAttributeCondition& Condition)
	{ return ASC->GetNumericAttribute(Condition.Attribute) < Condition.AtLeast; }))
	{
		return true;
	}
	const FGameplayTag& CostTag = UAbilitySystemGlobals::Get().ActivateFailCostTag;
	if (OptionalRelevantTags && CostTag.IsValid())
	{
		OptionalRelevantTags->AddTag(CostTag);
	}
	return false;
}

int32 UAgGameplayAbility::GetCharges() const
{
	const FAgAbilityData* Data = FindDefinition();
	const UAbilitySystemComponent* ASC = Data ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC)
	{
		return 0;
	}
	const int32 Used = ASC->GetActiveEffects(AgGameplayAbilityPrivate::CooldownQuery(this)).Num();
	return FMath::Max(Data->MaxCharges - Used, 0);
}

void UAgGameplayAbility::GetRechargeTimeRemainingAndDuration(float& TimeRemaining, float& CooldownDuration) const
{
	TimeRemaining = CooldownDuration = 0.f;
	const FAgAbilityData* Data = FindDefinition();
	const UAbilitySystemComponent* ASC = Data ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC)
	{
		return;
	}
	const TArray<float> Remaining = ASC->GetActiveEffectsTimeRemaining(AgGameplayAbilityPrivate::CooldownQuery(this));
	if (Remaining.IsEmpty())
	{
		return;
	}
	// 쿨다운 효과는 쿨다운 간격으로 줄지어 끝나므로, 가장 먼저 끝나는 효과까지가 다음 충전까지의 시간입니다.
	TimeRemaining = FMath::Min(Remaining);
	CooldownDuration = Data->Cooldown;
}

FGameplayEffectSpecHandle UAgGameplayAbility::MakeEffectSpec(const UGameplayEffect& Effect) const
{
	UAgAbilitySystemComponent* ASC = CurrentActorInfo ? Cast<UAgAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get()) : nullptr;
	if (!ASC)
	{
		return FGameplayEffectSpecHandle();
	}
	FGameplayEffectSpecHandle Spec = ASC->MakeEffectSpec(Effect, GetAbilityLevel(), MakeEffectContext(CurrentSpecHandle, CurrentActorInfo));
	if (Spec.IsValid())
	{
		ApplyAbilityTagsToGameplayEffectSpec(*Spec.Data, ASC->FindAbilitySpecFromHandle(CurrentSpecHandle));
	}
	return Spec;
}

FGameplayEffectSpecHandle UAgGameplayAbility::MakeDamageSpec(const FAgDamage& Damage, const TSharedPtr<FAgHitState>& HitState) const
{
	const FGameplayEffectSpecHandle Spec = MakeEffectSpec(EAgEffect::Damage);
	if (Spec.IsValid())
	{
		FAgGameplayEffectContext* Context = new FAgGameplayEffectContext();
		static_cast<FGameplayEffectContext&>(*Context) = *Spec.Data->GetContext().Get();
		Context->HitState = HitState;
		Spec.Data->SetContext(FGameplayEffectContextHandle(Context));
		Damage.AddToSpec(*Spec.Data);
	}
	return Spec;
}

const FGameplayEventData* UAgGameplayAbility::GetTriggerEvent() const
{
	// 엔진은 발동할 때 받은 실행 정보를 CurrentEventData에 복사해 둡니다. 이벤트 트리거는 부모 태그까지 맞춰 발동하고,
	// 태그 트리거로 발동해도 실행 정보가 오므로 출처를 가립니다.
	const bool bEventTrigger = Definition.AbilityTriggers.ContainsByPredicate([this](const FAbilityTriggerData& Trigger)
	{
		return Trigger.TriggerSource == EGameplayAbilityTriggerSource::GameplayEvent &&
		       CurrentEventData.EventTag.MatchesTag(Trigger.TriggerTag);
	});
	return bEventTrigger ? &CurrentEventData : nullptr;
}

void UAgGameplayAbility::RefreshTarget()
{
	if (!IsActive() || !Definition.Targeting)
	{
		return;
	}
	AActor* NewTarget = FindTargetingTarget(Definition);
	// 대상이 있어야 하는 행동은 대상 없이 이어 갈 수 없으므로, 다시 고를 후보가 없으면 지금 대상을 유지합니다.
	if (NewTarget || !Definition.Targeting->bRequired)
	{
		ActionTarget = NewTarget;
	}
}

void UAgGameplayAbility::SetActionTarget(AActor* NewTarget)
{
	if (IsActive())
	{
		ActionTarget = NewTarget;
	}
}

void UAgGameplayAbility::OnGameplayTaskActivated(UGameplayTask& Task)
{
	Super::OnGameplayTaskActivated(Task);
	// 엔진은 Activate를 마친 Task를 그동안 행동이 끝났어도 목록에 넣고 끝내지 않으므로, 여기서 끝냅니다.
	if (!IsActive())
	{
		Task.EndTask();
	}
}

void UAgGameplayAbility::OnGameplayTaskDeactivated(UGameplayTask& Task)
{
	Super::OnGameplayTaskDeactivated(Task);
	// 블루프린트에서 PlayMontageAndWait의 OnInterrupted를 CancelAbility에 잇는 것과 같습니다.
	const UAgAbilityTask_PlayMontage* Playback = Cast<UAgAbilityTask_PlayMontage>(&Task);
	if (Playback && Playback->WasInterrupted() && IsActive())
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return;
	}
	FinishIfTasksDone();
}

void UAgGameplayAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const FAgAbilityData* Data = FindDefinition();
	if (!Data)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	Definition = *Data;
	const TOptional<FAgAbilityTargeting>& Targeting = Definition.Targeting;

	// 대상 찾기는 락온 대상을 먼저 고르므로, 지난 실행의 대상을 비운 뒤 찾아야 락온 행동이 이전 대상을 다시 고르지 않습니다.
	ActionTarget.Reset();
	RefreshTarget();
	const FGameplayEventData* TriggerEvent = GetTriggerEvent();
	if (!Targeting && TriggerEvent)
	{
		ActionTarget = const_cast<AActor*>(TriggerEvent->Instigator.Get());
	}
	if ((Targeting && Targeting->bRequired && !ActionTarget.IsValid()) ||
		!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Task를 시작하는 중에 행동이 끝나고 다시 발동하면 사본이 바뀌므로 목록을 복사해 돕니다.
	{
		// End가 Tasks인 행동이 첫 Task가 끝나는 순간 끝나지 않도록, 발동 Task와 몽타주를 모두 시작한 뒤부터 봅니다.
		TGuardValue<bool> StartingTasks(bStartingTasks, true);
		const TArray<TInstancedStruct<FAgTaskParams>> Tasks = Definition.Tasks;
		const TOptional<FAgAbilityMontage> Montage = Definition.Montage;
		for (const TInstancedStruct<FAgTaskParams>& Item : Tasks)
		{
			if (!IsActive())
			{
				return;
			}
			if (UAbilityTask* Task = Item.Get().MakeTask(FAgTaskContext{*this}))
			{
				Task->ReadyForActivation();
			}
		}
		if (!IsActive())
		{
			return;
		}
		if (Montage)
		{
			Montage->MakeTask(*this)->ReadyForActivation();
		}
	}
	FinishIfTasksDone();
}

void UAgGameplayAbility::ApplyCost(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (!Definition.Cost.IsFree())
	{
		const FGameplayEffectSpecHandle Spec = MakeEffectSpec(EAgEffect::Resource);
		if (Spec.IsValid())
		{
			Definition.Cost.AddToSpec(*Spec.Data);
			ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
		}
	}
}

bool UAgGameplayAbility::CheckCooldown(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	return GetCharges() > 0;
}

void UAgGameplayAbility::ApplyCooldown(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo) const
{
	UAgAbilitySystemComponent* ASC = Cast<UAgAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get());
	if (Definition.Cooldown > 0.f && ASC)
	{
		// 걸린 쿨다운 효과 하나가 쓴 충전 하나입니다. 새 효과를 가장 늦게 끝나는 효과 뒤에 이어 붙여, 충전이 한 번에
		// 하나씩 돌아오고 흐르던 쿨다운이 다시 시작하지 않게 합니다.
		const float QueuedTime = FMath::Max(ASC->GetActiveEffectsTimeRemaining(AgGameplayAbilityPrivate::CooldownQuery(this)));
		// 출처로 이 어빌리티 인스턴스를 넣어, 행동마다의 쿨다운을 구분합니다.
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(this);
		const FGameplayEffectSpecHandle Spec = ASC->MakeEffectSpec(EAgEffect::Cooldown, GetAbilityLevel(Handle, ActorInfo), Context);
		if (Spec.IsValid())
		{
			Spec.Data->SetDuration(QueuedTime + Definition.Cooldown, true);
			ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
		}
	}
}

const FAgAbilityData* UAgGameplayAbility::FindDefinition() const
{
	const AAgCharacterBase* Character = CurrentActorInfo ? Cast<AAgCharacterBase>(CurrentActorInfo->AvatarActor.Get()) : nullptr;
	return Character ? Character->FindAbility(CurrentSpecHandle) : nullptr;
}

FGameplayEffectSpecHandle UAgGameplayAbility::MakeEffectSpec(EAgEffect Effect) const
{
	const UGameplayEffect* GameplayEffect = UAgAssetManager::Get().GetEffect(Effect);
	return GameplayEffect ? MakeEffectSpec(*GameplayEffect) : FGameplayEffectSpecHandle();
}

AActor* UAgGameplayAbility::FindTargetingTarget(const FAgAbilityData& Data) const
{
	AActor* Avatar = CurrentActorInfo ? CurrentActorInfo->AvatarActor.Get() : nullptr;
	return Data.Targeting && Avatar ? AgTargeting::FindTarget(*Data.Targeting, *Avatar) : nullptr;
}

void UAgGameplayAbility::FinishIfTasksDone()
{
	if (IsActive() && !bStartingTasks && Definition.End == EAgAbilityEnd::Tasks && ActiveTasks.IsEmpty())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}
