// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/AgTargeting.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilitySystemGlobals.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/AgCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/AgAssetManager.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "Data/AgEffectData.h"
#include "Data/AgTaskParams.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Ag.h"

namespace
{
	/** 이 어빌리티 인스턴스가 건 쿨다운 효과 */
	FGameplayEffectQuery CooldownQuery(const UObject* Source)
	{
		FGameplayEffectQuery Query =
			FGameplayEffectQuery::MakeQuery_MatchAnyEffectTags(FGameplayTagContainer(AgGameplayTags::Effect_Cooldown));
		Query.EffectSource = Source;
		return Query;
	}

	FGameplayAttribute ResourceAttribute(EAgResource Resource)
	{
		switch (Resource)
		{
			case EAgResource::SP:
				return UAgAttributeSet::GetSPAttribute();
			case EAgResource::MP:
				return UAgAttributeSet::GetMPAttribute();
			case EAgResource::UP:
				return UAgAttributeSet::GetUPAttribute();
		}
		return {};
	}
}

UAgGameplayAbility::UAgGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
	bRetriggerInstancedAbility = true;

	// 시간표 판정 틱은 시간표가 있는 동안만 켭니다(BeginTimeline, StopTimeline).
	TimelineTick.Target = this;
	TimelineTick.bCanEverTick = true;
	TimelineTick.TickGroup = TG_PrePhysics;
	TimelineTick.SetTickFunctionEnable(false);
}

void UAgGameplayAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	AAgCharacterBase* Character = ActorInfo ? Cast<AAgCharacterBase>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (Character && !HasAnyFlags(RF_ClassDefaultObject))
	{
		if (const FAgAbilityData* Data = FindDefinition(Spec.Handle, ActorInfo))
		{
			GrantedDefinition = *Data;
			ActivationRequiredTags = Data->Activation.RequiredTags;
			ActivationBlockedTags = Data->Activation.BlockedTags;
			ActivationOwnedTags = Data->Activation.OwnedTags;
			BlockAbilitiesWithTag = Data->Activation.BlockTags;
			CancelAbilitiesWithTag = Data->Activation.CancelTags;
		}

		// 몽타주가 진행된 뒤(루트 모션 몽타주는 이동 틱 안에서) 판정해, 같은 프레임의 공격 판정(TG_PostPhysics)보다 먼저 구간을 엽니다.
		UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
		USkeletalMeshComponent* Mesh = Character->GetMesh();
		TimelineTick.AddPrerequisite(Movement, Movement->PrimaryComponentTick);
		TimelineTick.AddPrerequisite(Mesh, Mesh->PrimaryComponentTick);
		TimelineTick.RegisterTickFunction(Character->GetLevel());
	}
	Super::OnGiveAbility(ActorInfo, Spec);
}

void UAgGameplayAbility::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	TimelineTick.UnRegisterTickFunction();
	Super::OnRemoveAbility(ActorInfo, Spec);
}

bool UAgGameplayAbility::CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	const FAgAbilityData* Data = FindDefinition(Handle, ActorInfo);
	const FGameplayAbilitySpec* Spec = ActorInfo && ActorInfo->AbilitySystemComponent.IsValid()
	                                       ? ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle)
	                                       : nullptr;
	const UAgGameplayAbility* Instance = Spec ? Cast<UAgGameplayAbility>(Spec->GetPrimaryInstance()) : nullptr;
	if (!Data || !Instance || !Instance->GrantedDefinition.HasSameGrantSettings(*Data))
	{
		UE_LOG(LogAg, Warning,
			TEXT("Ability %s: grant settings changed or unavailable; recreate the character before activation"),
			Data ? *Data->Id.ToString() : TEXT("None"));
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

	// 필수 대상이 없으면 발동하지 않습니다. 발동할 때(커밋 직전) 한 번 더 평가합니다.
	return !Data->bHasTargeting || !Data->Targeting.bRequired || FindTargetingTarget(*Data, ActorInfo);
}

bool UAgGameplayAbility::DoesAbilitySatisfyTagRequirements(const UAbilitySystemComponent& ASC, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	return !ASC.AreAbilityTagsBlocked(GrantedDefinition.Tags) &&
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

FAgResourceCostParams UAgGameplayAbility::GetCost() const
{
	const FAgAbilityData* Definition = IsActive() ? &ExecutionDefinition : FindDefinition(CurrentSpecHandle, CurrentActorInfo);
	return Definition && Definition->bHasCost ? Definition->Cost : FAgResourceCostParams();
}

FGameplayEffectSpecHandle UAgGameplayAbility::MakeEffectSpec(EAgEffect Effect) const
{
	return MakeEffectSpec(CurrentSpecHandle, CurrentActorInfo, UAgAssetManager::Get().GetEffect(Effect));
}

FGameplayEffectSpecHandle UAgGameplayAbility::MakeEffectSpec(const UGameplayEffect& Effect) const
{
	return MakeEffectSpec(CurrentSpecHandle, CurrentActorInfo, &Effect);
}

void UAgGameplayAbility::RequestTransition(const FAgTransition& Transition, int32 InstanceId)
{
	// 전환할 섹션이 몽타주에 있는지는 데이터를 읽을 때 시간표 검증이 봅니다.
	if (!OwnsMontage(InstanceId) || !Transition.IsSet())
	{
		return;
	}
	if (Transition.BlendTime < 0.f)
	{
		JumpToSection(Transition.Section);
		return;
	}

	// 새 재생은 대상을 다시 정합니다(평타의 타마다 대상). 이벤트의 Instigator는 바뀌지 않습니다.
	if (ExecutionDefinition.bHasTargeting)
	{
		ActionTarget = FindTargetingTarget(ExecutionDefinition, CurrentActorInfo);
		bTargetFromTargeting = ActionTarget.IsValid();
	}
	PlayMontage(Transition.Section, Transition.BlendTime);
}

void UAgGameplayAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const FAgAbilityData* Definition = FindDefinition(Handle, ActorInfo);
	const AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActorFromActorInfo());
	if (!Definition || !Character)
	{
		Finish(true);
		return;
	}
	ExecutionDefinition = *Definition;
	const UAgCharacterData* Data = Character->GetCharacterData();
	const FAgAbilityTargeting& Targeting = ExecutionDefinition.Targeting;
	StopDistance = ExecutionDefinition.bHasTargeting && Targeting.bHasStopDistance ? Targeting.StopDistance : Data->StopDistance;
	Montage = ExecutionDefinition.bHasMontage ? Data->FindMontage(ExecutionDefinition.Montage.MontageId) : nullptr;
	if (ExecutionDefinition.bHasMontage && !Montage)
	{
		UE_LOG(LogAg, Error, TEXT("Abilities.%s: montage %s is missing in the character's Montages"), *GetAbilityId().ToString(),
			*ExecutionDefinition.Montage.MontageId.ToString());
		Finish(true);
		return;
	}

	// 1. 대상을 정하고 커밋합니다. 필수 대상이 없거나 커밋에 실패하면 Task를 실행하지 않고 끝냅니다.
	ActionTarget = FindTargetingTarget(ExecutionDefinition, ActorInfo);
	bTargetFromTargeting = ActionTarget.IsValid();
	if (!ExecutionDefinition.bHasTargeting && IsEventTrigger(TriggerEventData))
	{
		ActionTarget = const_cast<AActor*>(TriggerEventData->Instigator.Get());
	}
	if ((ExecutionDefinition.bHasTargeting && Targeting.bRequired && !bTargetFromTargeting) ||
		!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		Finish(true);
		return;
	}

	// 2. 몽타주를 처음부터 재생하고 시작 위치의 구간·지점을 판정합니다. 재생에 실패하면 취소로 끝납니다.
	if (IsActive() && Montage)
	{
		PlayMontage(NAME_None, -1.f);
	}

	// 3. 발동 Task를 배열 순서대로 시작합니다. 재생과 같은 프레임이라 첫 화면과 첫 루트 모션보다 먼저입니다. 중간에 행동이 끝나면
	// 남은 Task는 실행하지 않습니다. Task를 시작하는 중에 행동이 끝나면 EndAbility가 실행 사본을 비우므로 목록을 복사해 돕니다.
	const TArray<FInstancedStruct> Tasks = ExecutionDefinition.Tasks;
	for (const FInstancedStruct& Item : Tasks)
	{
		if (!IsActive())
		{
			return;
		}
		if (UAbilityTask* Task = Item.Get<FAgTaskParams>().MakeTask(FAgTaskContext{*this, EAgTaskPlacement::Activation, MontageInstanceId}))
		{
			Task->ReadyForActivation();
		}
	}
}

void UAgGameplayAbility::EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 중복 호출과 스코프 락 동안의 지연은 엔진이 가리고, 남은 Task(발동·구간·지점)는 엔진이 끝냅니다. 실제로 끝날 때만 그 전에
	// 판정과 재생을 내려놓습니다. 재생 ID를 비우므로 엔진이 끝내는 Task의 늦은 전환 요청은 무시됩니다.
	if (IsEndAbilityValid(Handle, ActorInfo) && ScopeLockCount == 0)
	{
		const int32 InstanceId = MontageInstanceId;
		MontageInstanceId = INDEX_NONE;
		StopTimeline();

		// 끝나지 않은 재생은 몽타주의 블렌드 아웃으로 멈춥니다.
		UAnimInstance* Anim = CurrentActorInfo && InstanceId != INDEX_NONE ? CurrentActorInfo->GetAnimInstance() : nullptr;
		FAnimMontageInstance* Instance = Anim ? Anim->GetMontageInstanceForID(InstanceId) : nullptr;
		if (Instance && Montage && !Instance->IsStopped())
		{
			Instance->OnMontageBlendingOutStarted.Unbind();
			Instance->OnMontageEnded.Unbind();
			Instance->Stop(FAlphaBlend(Montage->BlendOut));
		}
		ActionTarget.Reset();
		bTargetFromTargeting = false;
		Montage = nullptr;
		ExecutionDefinition = {};
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UAgGameplayAbility::CheckCost(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	// 활성 스펙의 재발동 사전 검사도 여기로 들어오므로 새 요청의 현재 정의를 확인합니다.
	const FAgAbilityData* Data = FindDefinition(Handle, ActorInfo);
	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (Data && ASC && (!Data->bHasCost || ASC->GetNumericAttribute(ResourceAttribute(Data->Cost.Resource)) >= Data->Cost.Amount))
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
	if (ExecutionDefinition.bHasCost && ExecutionDefinition.Cost.Amount > 0.f)
	{
		// 비용은 자원 효과를 음수로 겁니다.
		const FGameplayEffectSpecHandle Spec = MakeEffectSpec(Handle, ActorInfo, UAgAssetManager::Get().GetEffect(EAgEffect::Resource));
		if (Spec.IsValid())
		{
			ExecutionDefinition.Cost.AddToSpec(*Spec.Data);
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
	if (ExecutionDefinition.Cooldown > 0.f && ASC)
	{
		// 출처로 이 어빌리티 인스턴스를 넣어, 행동마다의 쿨다운을 구분합니다.
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(this);
		const FGameplayEffectSpecHandle Spec = ASC->MakeEffectSpec(EAgEffect::Cooldown, GetAbilityLevel(Handle, ActorInfo), Context);
		if (Spec.IsValid())
		{
			Spec.Data->SetDuration(ExecutionDefinition.Cooldown, /*bLockDuration*/ true);
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
	// 컨텍스트에는 이 어빌리티 인스턴스가 들어갑니다(피격 처리가 극한 회피 구간 효과를 건 행동의 비용을 읽음).
	FGameplayEffectSpecHandle Spec = ASC->MakeEffectSpec(*Effect, GetAbilityLevel(Handle, ActorInfo), MakeEffectContext(Handle, ActorInfo));
	if (Spec.IsValid())
	{
		FGameplayAbilitySpec* AbilitySpec = ASC->FindAbilitySpecFromHandle(Handle);
		ApplyAbilityTagsToGameplayEffectSpec(*Spec.Data, AbilitySpec);
		if (AbilitySpec)
		{
			Spec.Data->SetByCallerTagMagnitudes = AbilitySpec->SetByCallerTagMagnitudes;
		}
	}
	return Spec;
}

AActor* UAgGameplayAbility::FindTargetingTarget(const FAgAbilityData& Data, const FGameplayAbilityActorInfo* ActorInfo) const
{
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	return Data.bHasTargeting && Avatar ? AgTargeting::FindTarget(Data.Targeting, *Avatar) : nullptr;
}

bool UAgGameplayAbility::IsEventTrigger(const FGameplayEventData* TriggerEventData) const
{
	// 엔진은 이벤트 트리거를 부모 태그까지 맞춰 발동합니다. 태그 트리거로 발동해도 실행 정보가 오므로 출처를 가립니다.
	return TriggerEventData && ExecutionDefinition.Activation.Triggers.ContainsByPredicate([TriggerEventData](const FAgAbilityTrigger& Trigger)
	{
		return Trigger.Source == EAgTriggerSource::GameplayEvent &&
		       TriggerEventData->EventTag.MatchesTag(Trigger.Tag);
	});
}

void UAgGameplayAbility::Finish(bool bCancelled)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bCancelled);
}

void UAgGameplayAbility::PlayMontage(FName Section, float BlendTime)
{
	UAgAbilitySystemComponent* ASC = CastChecked<UAgAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());

	// 다시 재생하면 이전 재생의 구간을 끝냅니다. 지점 Task는 남습니다. 이전 재생은 새 재생의 블렌드로 끝납니다. 끝나는 구간이
	// 요청하는 전환은 이전 재생의 것이므로 받지 않도록 재생 ID를 먼저 비웁니다.
	const int32 PreviousInstanceId = MontageInstanceId;
	MontageInstanceId = INDEX_NONE;
	EndTimeline();
	if (!IsActive())
	{
		return;
	}
	FAnimMontageInstance* Instance = ASC->PlayMontageWithBlendIn(*this, *Montage, BlendTime, Section);
	if (!IsActive())
	{
		// 이전 재생이 끊기며 부른 콜백이 이 행동을 끝냈습니다. 새 재생은 ASC가 멈췄습니다.
		return;
	}
	if (!Instance)
	{
		UE_LOG(LogAg, Error, TEXT("Abilities.%s: failed to play %s (section %s)"), *GetAbilityId().ToString(), *Montage->GetName(),
			*Section.ToString());
		// 이전 재생이 남아 있으면 행동을 끝낼 때 멈추도록 다시 맡습니다.
		MontageInstanceId = PreviousInstanceId;
		Finish(true);
		return;
	}
	const int32 InstanceId = Instance->GetInstanceID();
	MontageInstanceId = InstanceId;
	Instance->OnMontageBlendingOutStarted =
		FOnMontageBlendingOutStarted::CreateUObject(this, &ThisClass::HandleMontageBlendingOut, InstanceId);
	Instance->OnMontageEnded = FOnMontageEnded::CreateUObject(this, &ThisClass::HandleMontageEnded, InstanceId);

	// 시작 위치에 걸친 구간과 그 위치의 지점은 여기서 바로 실행합니다. 그 실행에서 전환하거나 행동이 끝날 수 있습니다.
	BeginTimeline(*Instance);
}

void UAgGameplayAbility::JumpToSection(FName Section)
{
	UAnimInstance* Anim = CurrentActorInfo ? CurrentActorInfo->GetAnimInstance() : nullptr;
	const FAnimMontageInstance* Instance =
		Anim && MontageInstanceId != INDEX_NONE ? Anim->GetMontageInstanceForID(MontageInstanceId) : nullptr;
	if (!Instance || !Montage)
	{
		return;
	}

	// 엔진은 블렌드 아웃 중인 재생을 옮기지 않으므로, 위치가 바뀌었을 때만 시간표를 다시 맞춥니다. 건너뛴 지점은 실행하지 않고
	// 옮긴 위치의 지점은 실행합니다.
	const float Before = Instance->GetPosition();
	Anim->Montage_JumpToSection(Section, Montage);
	if (IsActive() && Instance->GetPosition() != Before)
	{
		EvaluateTimeline(Instance->GetPosition(), EAgTimelineMode::Jump);
	}
}

void UAgGameplayAbility::HandleMontageBlendingOut(UAnimMontage* EndedMontage, bool bInterrupted, int32 InstanceId)
{
	// 다른 몽타주에 끊기면 그때 취소합니다. 이 행동이 다시 재생해 끝나는 이전 재생은 지금 재생이 아니라 무시합니다.
	if (bInterrupted && OwnsMontage(InstanceId))
	{
		FinishPlayback(true);
	}
}

void UAgGameplayAbility::HandleMontageEnded(UAnimMontage* EndedMontage, bool bInterrupted, int32 InstanceId)
{
	if (OwnsMontage(InstanceId))
	{
		FinishPlayback(bInterrupted);
	}
}

void UAgGameplayAbility::FinishPlayback(bool bInterrupted)
{
	// 끝나는 구간이 요청하는 전환을 받지 않도록 재생 ID를 먼저 비웁니다.
	MontageInstanceId = INDEX_NONE;
	EndTimeline();
	if (ExecutionDefinition.End == EAgAbilityEnd::Montage)
	{
		Finish(bInterrupted);
	}
}
