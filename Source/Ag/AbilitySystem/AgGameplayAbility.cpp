// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/Tasks/AgAbilityTask_HoldTag.h"
#include "AbilitySystem/Tasks/AgAbilityTask_Dash.h"
#include "AbilitySystem/Tasks/AgAbilityTask_TimeStop.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitMovementModeChange.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Animation/AgMontageTimelineComponent.h"
#include "Animation/AgRootMotionModifier_MotionWarping.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "AbilitySystemGlobals.h"
#include "Character/AgCharacterBase.h"
#include "Character/AgPlayerCharacter.h"
#include "Character/AgBossCharacter.h"
#include "AbilitySystem/Tasks/AgAbilityTask_LockOn.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "Data/AgEffectData.h"
#include "Data/AgMontageTimeline.h"
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
}

AAgCharacterBase* UAgGameplayAbility::GetAgCharacter() const
{
	return Cast<AAgCharacterBase>(GetAvatarActorFromActorInfo());
}

const FAgAbilityData* UAgGameplayAbility::FindDefinition(
    FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const
{
	const AAgCharacterBase* Character = ActorInfo ? Cast<AAgCharacterBase>(ActorInfo->AvatarActor.Get()) : nullptr;
	return Character ? Character->FindAbility(Handle) : nullptr;
}

void UAgGameplayAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
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
	}
	Super::OnGiveAbility(ActorInfo, Spec);
}

void UAgGameplayAbility::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	if (AAgCharacterBase* Character = ActorInfo ? Cast<AAgCharacterBase>(ActorInfo->AvatarActor.Get()) : nullptr)
	{
		Character->ForgetAbility(Spec.Handle);
	}
	Super::OnRemoveAbility(ActorInfo, Spec);
	GrantedDefinition = {};
}

bool UAgGameplayAbility::CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* RelevantTags) const
{
	const FAgAbilityData* Data = FindDefinition(Handle, ActorInfo);
	const FGameplayAbilitySpec* Spec = ActorInfo && ActorInfo->AbilitySystemComponent.IsValid()
	                                       ? ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle)
	                                       : nullptr;
	const UAgGameplayAbility* Instance = Spec ? Cast<UAgGameplayAbility>(Spec->GetPrimaryInstance()) : nullptr;
	if (!Data || !Instance || !Instance->GrantedDefinition.HasSameGrantSettings(*Data))
	{
		UE_LOG(LogAg, Warning,
		    TEXT("Ability %s: grant settings changed or unavailable; recreate character or regrant spec before activation"),
		    Data ? *Data->Id.ToString() : TEXT("None"));
		return false;
	}
	if (Spec->IsActive() && !Data->Activation.bAllowRetrigger)
	{
		return false;
	}
	return CheckStartConditions(*Data, ActorInfo) && Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, RelevantTags);
}

bool UAgGameplayAbility::DoesAbilitySatisfyTagRequirements(const UAbilitySystemComponent& ASC, const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags, FGameplayTagContainer* RelevantTags) const
{
	return !ASC.AreAbilityTagsBlocked(GrantedDefinition.Tags) &&
	       Super::DoesAbilitySatisfyTagRequirements(ASC, SourceTags, TargetTags, RelevantTags);
}

AAgCharacterBase* UAgGameplayAbility::GetLockOnTarget() const
{
	return IsActive() && LockOnTask ? LockOnTask->GetTarget() : nullptr;
}

bool UAgGameplayAbility::CheckStartConditions(const FAgAbilityData& Data, const FGameplayAbilityActorInfo* ActorInfo) const
{
	const AAgCharacterBase* Character = ActorInfo ? Cast<AAgCharacterBase>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!Character)
	{
		return false;
	}
	const AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(Character);
	if (Data.Id == TEXT("Jump"))
	{
		return Player && Player->CanJump();
	}
	if (Data.Id == TEXT("JumpAttack"))
	{
		return Character->GetCharacterMovement()->IsFalling();
	}
	if (Data.Id == TEXT("Guard"))
	{
		return Player && Player->IsGuardHeld();
	}
	if (Data.Id == TEXT("Execution"))
	{
		return FindExecutionTarget(Player) != nullptr;
	}
	if (Data.Id == TEXT("LeapSlam"))
	{
		const AAgBossCharacter* Boss = Cast<AAgBossCharacter>(Character);
		return Boss && Boss->GetTarget();
	}
	if (Data.Id == TEXT("LockOn"))
	{
		return Player && UAgAbilityTask_LockOn::FindTarget(*Player);
	}
	return true;
}

bool UAgGameplayAbility::CheckResources(const FAgAbilityData& Data, const FGameplayAbilityActorInfo* ActorInfo) const
{
	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC)
	{
		return false;
	}
	if (Data.bHasCost && ASC->GetNumericAttribute(ResourceAttribute(Data.Cost.Resource)) < Data.Cost.Amount)
	{
		return false;
	}
	if (Data.Tasks.bHasDrainResource)
	{
		const FAgDrainResourceParams& Drain = Data.Tasks.DrainResource;
		const float Remaining = ASC->GetNumericAttribute(ResourceAttribute(Drain.Resource)) -
		                        (Data.bHasCost && Data.Cost.Resource == Drain.Resource ? Data.Cost.Amount : 0.f);
		if (Remaining <= 0.f)
		{
			return false;
		}
	}
	return true;
}

bool UAgGameplayAbility::CheckCost(
    FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* RelevantTags) const
{
	// 활성 스펙의 재발동 사전 검사도 여기로 들어오므로 새 요청의 현재 정의를 확인합니다.
	const FAgAbilityData* Data = FindDefinition(Handle, ActorInfo);
	if (Data && CheckResources(*Data, ActorInfo))
	{
		return true;
	}
	const FGameplayTag& CostTag = UAbilitySystemGlobals::Get().ActivateFailCostTag;
	if (RelevantTags && CostTag.IsValid())
	{
		RelevantTags->AddTag(CostTag);
	}
	return false;
}

FGameplayEffectSpecHandle UAgGameplayAbility::MakeEffectSpec(EAgEffect Effect) const
{
	return MakeEffectSpec(CurrentSpecHandle, CurrentActorInfo, Effect);
}

FGameplayEffectSpecHandle UAgGameplayAbility::MakeEffectSpec(
    FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, EAgEffect Effect) const
{
	UAgAbilitySystemComponent* ASC = ActorInfo ? Cast<UAgAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get()) : nullptr;
	if (!ASC)
	{
		return FGameplayEffectSpecHandle();
	}
	FGameplayEffectSpecHandle Spec = ASC->MakeEffectSpec(Effect, GetAbilityLevel(Handle, ActorInfo), MakeEffectContext(Handle, ActorInfo));
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

FAgResourceCostParams UAgGameplayAbility::GetCost() const
{
	const FAgAbilityData* Definition = IsActive() ? &ExecutionDefinition : FindDefinition(CurrentSpecHandle, CurrentActorInfo);
	return Definition && Definition->bHasCost ? Definition->Cost : FAgResourceCostParams();
}

void UAgGameplayAbility::ApplyCost(
    FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (ExecutionDefinition.bHasCost && ExecutionDefinition.Cost.Amount > 0.f)
	{
		// 비용은 자원 효과를 음수로 겁니다.
		const FGameplayEffectSpecHandle Spec = MakeEffectSpec(Handle, ActorInfo, EAgEffect::Resource);
		if (Spec.IsValid())
		{
			ExecutionDefinition.Cost.AddToSpec(*Spec.Data);
			ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
		}
	}
}

bool UAgGameplayAbility::CheckCooldown(
    FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* RelevantTags) const
{
	const FGameplayAbilitySpec* Spec = ActorInfo ? ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle) : nullptr;
	return Spec && ActorInfo->AbilitySystemComponent->GetActiveEffects(CooldownQuery(Spec->GetPrimaryInstance())).IsEmpty();
}

void UAgGameplayAbility::ApplyCooldown(
    FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo) const
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

void UAgGameplayAbility::GetCooldownTimeRemainingAndDuration(
    FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, float& Remaining, float& Duration) const
{
	Remaining = Duration = 0.f;
	const FGameplayAbilitySpec* Spec = ActorInfo ? ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle) : nullptr;
	if (!Spec)
	{
		return;
	}
	for (const TPair<float, float>& Entry :
	    ActorInfo->AbilitySystemComponent->GetActiveEffectsTimeRemainingAndDuration(CooldownQuery(Spec->GetPrimaryInstance())))
	{
		if (Entry.Key > Remaining)
		{
			Remaining = Entry.Key;
			Duration = Entry.Value;
		}
	}
}

void UAgGameplayAbility::BeginBusy()
{
	if (!BusyTag)
	{
		BusyTag = UAgAbilityTask_HoldTag::HoldTag(this, AgGameplayTags::State_Busy);
		BusyTag->ReadyForActivation();
	}
}

void UAgGameplayAbility::EndBusy()
{
	UAgAbilityTask_HoldTag* Task = BusyTag.Get();
	BusyTag = nullptr;
	if (Task)
	{
		Task->EndTask();
	}
}

void UAgGameplayAbility::Finish(bool bCancelled)
{
	if (IsActive() && !bEnding)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bCancelled);
	}
}

bool UAgGameplayAbility::PlayMotion(FName Id, bool bPlayerAction, float Rate, FName Section, float TranslationScale, float StartTime)
{
	UAnimMontage* Montage = ExecutionMontages.FindRef(Id);
	if (!Montage || !IsActive())
	{
		Finish(true);
		return false;
	}
	ClearMotion(false);
	if (!IsActive())
	{
		return false;
	}
	Motion.Montage = Montage;
	Motion.MontageId = Id;
	Motion.bBlocksMovement = bPlayerAction;
	if (bPlayerAction)
	{
		BeginBusy();
	}
	if (!IsActive() || bEnding)
	{
		return false;
	}
	MotionTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
	    this, NAME_None, Montage, Rate, Section, true, TranslationScale, StartTime);
	MotionTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMotionCompleted);
	MotionTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMotionCancelled);
	MotionTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMotionCancelled);
	MotionTask->OnBlendOut.AddDynamic(this, &ThisClass::HandleMotionBlendOut);
	UAbilityTask_PlayMontageAndWait* StartedTask = MotionTask;
	StartedTask->ReadyForActivation();
	if (!IsActive() || MotionTask != StartedTask || GetAbilitySystemComponentFromActorInfo()->GetAnimatingAbility() != this)
	{
		return false;
	}
	UAnimInstance* Anim = GetActorInfo().GetAnimInstance();
	FAnimMontageInstance* Instance = Anim ? Anim->GetActiveInstanceForMontage(Montage) : nullptr;
	if (!Instance)
	{
		Finish(true);
		return false;
	}
	const int32 InstanceId = Instance->GetInstanceID();
	BindMontageInstance(*Instance);
	if (OwnsMontage(InstanceId))
	{
		// 시작 위치에 걸친 구간과 그 위치의 지점은 여기서 바로 실행합니다. 이 알림에서 재생 자체가 끝날 수 있습니다.
		GetAgCharacter()->GetMontageTimeline()->BeginPlayback(InstanceId);
	}
	if (OwnsMontage(InstanceId) && !Anim->GetMontageInstanceForID(InstanceId))
	{
		Finish(true);
	}
	return MotionTask == StartedTask && OwnsMontage(InstanceId);
}

void UAgGameplayAbility::JumpToMotionSection(FName Section)
{
	MontageJumpToSection(Section);
	if (IsActive() && !bEnding && Motion.InstanceId != INDEX_NONE)
	{
		GetAgCharacter()->GetMontageTimeline()->Resync(Motion.InstanceId);
	}
}

void UAgGameplayAbility::BindMontageInstance(const FAnimMontageInstance& Instance)
{
	if (IsActive() && Motion.Montage == Instance.Montage)
	{
		Motion.InstanceId = Instance.GetInstanceID();
		Motion.TimelineHandle = GetAgCharacter()->GetMontageTimeline()->OnTimelineEvent.AddUObject(this, &ThisClass::HandleTimelineEvent);
		// 공격, 발사, 워핑이 있는 모션은 회전을 직접 정합니다.
		const FAgMontageTimeline* Timeline = UAgMontageTimelineComponent::FindTimeline(*Instance.Montage);
		Motion.bControlsFacing =
		    Timeline && (!Timeline->Attack.IsEmpty() || !Timeline->SpawnProjectile.IsEmpty() || !Timeline->MotionWarping.IsEmpty());
		if (Motion.bControlsFacing)
		{
			Motion.Target = GetAgCharacter()->FindAttackTarget();
		}
		UpdateMotionConstraints();
	}
}

void UAgGameplayAbility::UpdateMotionConstraints()
{
	if (!IsActive() || bEnding || Motion.InstanceId == INDEX_NONE)
	{
		return;
	}
	FAgCharacterMotionConstraints Constraints;
	Constraints.bControlsFacing = Motion.bControlsFacing;
	Constraints.bTranslationFromVelocity = bRootMotionTranslationFromVelocity;
	if (Motion.bControlsFacing && GetAbilityId() != TEXT("Execution"))
	{
		Constraints.StopTarget = Motion.Target.Get();
		Constraints.StopDistance = WarpStopDistance;
	}
	GetAgCharacter()->SetMotionConstraints(*this, Constraints);
}

void UAgGameplayAbility::EndWindows()
{
	TArray<FAgAbilityWindowExecution> Ending = MoveTemp(Windows);
	Windows.Reset();
	for (FAgAbilityWindowExecution& Window : Ending)
	{
		if (Window.Task && !Window.Task->IsFinished())
		{
			Window.Task->EndTask();
		}
	}
}

void UAgGameplayAbility::ClearMotion(bool bStop)
{
	const FAgAbilityMontageExecution Previous = Motion;
	Motion = {};
	if (AAgCharacterBase* Character = GetAgCharacter())
	{
		Character->GetMontageTimeline()->OnTimelineEvent.Remove(Previous.TimelineHandle);
		Character->ClearMotionConstraints(*this);
	}
	UAbilityTask_PlayMontageAndWait* PreviousTask = MotionTask.Get();
	MotionTask = nullptr;
	if (PreviousTask)
	{
		PreviousTask->OnCompleted.RemoveAll(this);
		PreviousTask->OnInterrupted.RemoveAll(this);
		PreviousTask->OnCancelled.RemoveAll(this);
		PreviousTask->OnBlendOut.RemoveAll(this);
		PreviousTask->EndTask();
	}
	EndWindows();
	TArray<TObjectPtr<UAgRootMotionModifier_MotionWarping>> PreviousWarps = MoveTemp(Warps);
	Warps.Reset();
	for (UAgRootMotionModifier_MotionWarping* Warp : PreviousWarps)
	{
		if (Warp)
		{
			Warp->SetState(ERootMotionModifierState::MarkedForRemoval);
		}
	}
	if (UAnimInstance* Anim = GetActorInfo().GetAnimInstance(); Anim && Previous.Montage)
	{
		if (const FAnimMontageInstance* Instance = Anim->GetMontageInstanceForID(Previous.InstanceId); Instance && Instance->IsActive())
		{
			if (bHoldingDash)
			{
				Anim->Montage_Resume(Previous.Montage);
			}
			if (bStop)
			{
				Anim->Montage_StopWithBlendOut(Previous.Montage->GetBlendOutArgs(), Previous.Montage);
			}
		}
	}
	bHoldingDash = false;
	DashTask = nullptr;
}

void UAgGameplayAbility::Listen(FGameplayTag Event)
{
	UAbilityTask_WaitGameplayEvent* Task = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, Event, nullptr, false, true);
	Task->EventReceived.AddDynamic(this, &ThisClass::HandleEvent);
	Task->ReadyForActivation();
}

void UAgGameplayAbility::WaitForLanding()
{
	UAbilityTask_WaitMovementModeChange* Task = UAbilityTask_WaitMovementModeChange::CreateWaitMovementModeChange(this, MOVE_Walking);
	Task->OnChange.AddDynamic(this, &ThisClass::HandleLanded);
	Task->ReadyForActivation();
}

void UAgGameplayAbility::EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
    FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bEnding || !IsActive())
	{
		return;
	}
	if (ScopeLockCount > 0)
	{
		Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
		return;
	}
	bEnding = true;
	CloseParryWindow();
	if (DrainEffect.IsValid())
	{
		GetAbilitySystemComponentFromActorInfo()->RemoveActiveGameplayEffect(DrainEffect);
		DrainEffect.Invalidate();
	}
	ClearMotion(true);
	EndBusy();
	if (AAgCharacterBase* Character = GetAgCharacter())
	{
		if (GetAbilityId() == TEXT("Jump"))
		{
			Character->StopJumping();
		}
		if (GetAbilityId() == TEXT("Groggy"))
		{
			CastChecked<AAgBossCharacter>(Character)->SetGroggyTimerRunning(false);
		}
	}
	GuardingTag = nullptr;
	DownedTag = nullptr;
	UltimateTimeStop = nullptr;
	LockOnTask = nullptr;
	GroggyWait = nullptr;
	ExecutionMontages.Reset();
	ExecutionProjectiles.Reset();
	ComboMontageIds.Reset();
	ExecutionTarget.Reset();
	ExecutionDefinition = {};
	bRootMotionTranslationFromVelocity = false;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
