// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystem/AgGameplayEffectContext.h"
#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/AgTargeting.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilitySystemGlobals.h"
#include "Character/AgCharacterBase.h"
#include "Core/AgAssetManager.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "Data/AgAttackData.h"
#include "Data/AgEffectData.h"
#include "Data/AgTaskParams.h"
#include "Ag.h"

namespace
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
		if (const FAgAbilityData* Data = FindDefinition(Spec.Handle, ActorInfo))
		{
			Definition = *Data;
			ActivationRequiredTags = Data->Activation.RequiredTags;
			ActivationBlockedTags = Data->Activation.BlockedTags;
			ActivationOwnedTags = Data->Activation.OwnedTags;
			BlockAbilitiesWithTag = Data->Activation.BlockTags;
			CancelAbilitiesWithTag = Data->Activation.CancelTags;
		}
	}
	Super::OnGiveAbility(ActorInfo, Spec);
}

bool UAgGameplayAbility::CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	const FAgAbilityData* Data = FindDefinition(Handle, ActorInfo);
	const FGameplayAbilitySpec* Spec = ActorInfo && ActorInfo->AbilitySystemComponent.IsValid()
	                                       ? ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle)
	                                       : nullptr;
	const UAgGameplayAbility* Instance = Spec ? Cast<UAgGameplayAbility>(Spec->GetPrimaryInstance()) : nullptr;
	if (!Data || !Instance || !Instance->Definition.HasSameGrantSettings(*Data))
	{
		UE_LOG(LogAg, Warning,
			TEXT("Ability %s: grant settings changed or unavailable; recreate the character before activation"),
			Instance ? *Instance->GetAbilityId().ToString() : TEXT("None"));
		return false;
	}
	if (Spec->IsActive() && !Data->Activation.bAllowRetrigger)
	{
		return false;
	}
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	return !Data->Targeting || !Data->Targeting->bRequired || FindTargetingTarget(*Data, ActorInfo);
}

bool UAgGameplayAbility::DoesAbilitySatisfyTagRequirements(const UAbilitySystemComponent& ASC, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	// 엔진의 차단 검사는 클래스의 어빌리티 태그를 보므로, 스펙에 둔 행동 태그의 차단도 확인합니다.
	return !ASC.AreAbilityTagsBlocked(Definition.Tags) &&
	       Super::DoesAbilitySatisfyTagRequirements(ASC, SourceTags, TargetTags, OptionalRelevantTags);
}

void UAgGameplayAbility::GetCooldownTimeRemainingAndDuration(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, float& TimeRemaining, float& CooldownDuration) const
{
	TimeRemaining = CooldownDuration = 0.f;
	const FGameplayAbilitySpec* Spec = ActorInfo ? ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle) : nullptr;
	if (!Spec)
	{
		return;
	}
	for (const TPair<float, float>& Entry :
		ActorInfo->AbilitySystemComponent->GetActiveEffectsTimeRemainingAndDuration(CooldownQuery(Spec->GetPrimaryInstance())))
	{
		if (Entry.Key > TimeRemaining)
		{
			TimeRemaining = Entry.Key;
			CooldownDuration = Entry.Value;
		}
	}
}

FAgResourceCost UAgGameplayAbility::GetCost() const
{
	const FAgAbilityData* Data = IsActive() ? &Definition : FindDefinition(CurrentSpecHandle, CurrentActorInfo);
	return Data ? Data->Cost : FAgResourceCost();
}

FGameplayEffectSpecHandle UAgGameplayAbility::MakeEffectSpec(EAgEffect Effect) const
{
	return MakeEffectSpec(CurrentSpecHandle, CurrentActorInfo, UAgAssetManager::Get().GetEffect(Effect));
}

FGameplayEffectSpecHandle UAgGameplayAbility::MakeEffectSpec(const UGameplayEffect& Effect) const
{
	return MakeEffectSpec(CurrentSpecHandle, CurrentActorInfo, &Effect);
}

FGameplayEffectSpecHandle UAgGameplayAbility::MakeDamageSpec(const FAgDamage& Damage, const TSharedPtr<FAgHitRewardState>& Reward) const
{
	const FGameplayEffectSpecHandle Spec = MakeEffectSpec(EAgEffect::Damage);
	if (Spec.IsValid())
	{
		FAgGameplayEffectContext* Context = new FAgGameplayEffectContext();
		static_cast<FGameplayEffectContext&>(*Context) = *Spec.Data->GetContext().Get();
		Context->HitReward = Reward;
		Spec.Data->SetContext(FGameplayEffectContextHandle(Context));
		Damage.AddToSpec(*Spec.Data);
	}
	return Spec;
}

void UAgGameplayAbility::RefreshTarget()
{
	if (IsActive() && Definition.Targeting)
	{
		ActionTarget = FindTargetingTarget(Definition, CurrentActorInfo);
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
	FinishIfTasksDone();
}

void UAgGameplayAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const FAgAbilityData* Data = FindDefinition(Handle, ActorInfo);
	if (!Data)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	Definition = *Data;
	const TOptional<FAgAbilityTargeting>& Targeting = Definition.Targeting;

	RefreshTarget();
	const bool bEventTrigger = IsEventTrigger(TriggerEventData);
	TriggerEventTag = bEventTrigger ? TriggerEventData->EventTag : FGameplayTag();
	if (!Targeting && bEventTrigger)
	{
		ActionTarget = const_cast<AActor*>(TriggerEventData->Instigator.Get());
	}
	if ((Targeting && Targeting->bRequired && !ActionTarget.IsValid()) ||
		!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Task를 시작하는 중에 행동이 끝나고 다시 발동하면 사본이 바뀌므로 목록을 복사해 돕니다.
	{
		// End가 Tasks인 행동이 첫 Task가 끝나는 순간 끝나지 않도록, 발동 Task를 모두 시작한 뒤부터 봅니다.
		TGuardValue<bool> StartingTasks(bStartingTasks, true);
		const TArray<TInstancedStruct<FAgTaskParams>> Tasks = Definition.Tasks;
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
	}
	FinishIfTasksDone();
}

void UAgGameplayAbility::EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 중복 호출과 스코프 락 동안의 지연은 엔진이 가리고 남은 Task는 엔진이 끝내므로, 실제로 끝날 때만 실행 상태를
	// 내려놓습니다.
	if (IsEndAbilityValid(Handle, ActorInfo) && ScopeLockCount == 0)
	{
		ActionTarget.Reset();
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UAgGameplayAbility::CheckCost(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	// 활성 스펙의 재발동 사전 검사도 여기로 들어오므로 새 요청의 현재 정의를 확인합니다.
	const FAgAbilityData* Data = FindDefinition(Handle, ActorInfo);
	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (Data && ASC && ASC->GetNumericAttribute(UAgAttributeSet::GetSPAttribute()) >= Data->Cost.SP &&
		ASC->GetNumericAttribute(UAgAttributeSet::GetMPAttribute()) >= Data->Cost.MP &&
		ASC->GetNumericAttribute(UAgAttributeSet::GetUPAttribute()) >= Data->Cost.UP)
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

void UAgGameplayAbility::ApplyCost(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (!Definition.Cost.IsFree())
	{
		const FGameplayEffectSpecHandle Spec = MakeEffectSpec(Handle, ActorInfo, UAgAssetManager::Get().GetEffect(EAgEffect::Resource));
		if (Spec.IsValid())
		{
			Definition.Cost.AddToSpec(*Spec.Data);
			ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
		}
	}
}

bool UAgGameplayAbility::CheckCooldown(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	const FGameplayAbilitySpec* Spec = ActorInfo ? ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle) : nullptr;
	return Spec && ActorInfo->AbilitySystemComponent->GetActiveEffects(CooldownQuery(Spec->GetPrimaryInstance())).IsEmpty();
}

void UAgGameplayAbility::ApplyCooldown(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo) const
{
	UAgAbilitySystemComponent* ASC = Cast<UAgAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get());
	if (Definition.Cooldown > 0.f && ASC)
	{
		// 출처로 이 어빌리티 인스턴스를 넣어, 행동마다의 쿨다운을 구분합니다.
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(this);
		const FGameplayEffectSpecHandle Spec = ASC->MakeEffectSpec(EAgEffect::Cooldown, GetAbilityLevel(Handle, ActorInfo), Context);
		if (Spec.IsValid())
		{
			Spec.Data->SetDuration(Definition.Cooldown, true);
			ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
		}
	}
}

const FAgAbilityData* UAgGameplayAbility::FindDefinition(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const
{
	const AAgCharacterBase* Character = ActorInfo ? Cast<AAgCharacterBase>(ActorInfo->AvatarActor.Get()) : nullptr;
	return Character ? Character->FindAbility(Handle) : nullptr;
}

FGameplayEffectSpecHandle UAgGameplayAbility::MakeEffectSpec(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const UGameplayEffect* Effect) const
{
	UAgAbilitySystemComponent* ASC = ActorInfo ? Cast<UAgAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get()) : nullptr;
	if (!ASC || !Effect)
	{
		return FGameplayEffectSpecHandle();
	}
	FGameplayEffectSpecHandle Spec = ASC->MakeEffectSpec(*Effect, GetAbilityLevel(Handle, ActorInfo), MakeEffectContext(Handle, ActorInfo));
	if (Spec.IsValid())
	{
		ApplyAbilityTagsToGameplayEffectSpec(*Spec.Data, ASC->FindAbilitySpecFromHandle(Handle));
	}
	return Spec;
}

AActor* UAgGameplayAbility::FindTargetingTarget(const FAgAbilityData& Data, const FGameplayAbilityActorInfo* ActorInfo) const
{
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	return Data.Targeting && Avatar ? AgTargeting::FindTarget(*Data.Targeting, *Avatar) : nullptr;
}

bool UAgGameplayAbility::IsEventTrigger(const FGameplayEventData* TriggerEventData) const
{
	// 엔진은 이벤트 트리거를 부모 태그까지 맞춰 발동합니다. 태그 트리거로 발동해도 실행 정보가 오므로 출처를 가립니다.
	return TriggerEventData && Definition.Activation.Triggers.ContainsByPredicate([TriggerEventData](const FAgAbilityTrigger& Trigger)
	{
		return Trigger.Source == EAgTriggerSource::GameplayEvent &&
		       TriggerEventData->EventTag.MatchesTag(Trigger.Tag);
	});
}

void UAgGameplayAbility::FinishIfTasksDone()
{
	if (IsActive() && !bStartingTasks && Definition.End == EAgAbilityEnd::Tasks && ActiveTasks.IsEmpty())
	{
		K2_EndAbility();
	}
}
