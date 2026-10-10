// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgBossPatternAbility.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/Tasks/AgAbilityTask_WaitMontagePosition.h"
#include "AbilitySystemComponent.h"
#include "Ag.h"
#include "Animation/AgAnimNotify_GameplayEvent.h"
#include "Animation/AgAnimNotifyState_AttackWindow.h"
#include "Animation/AnimMontage.h"
#include "Character/AgBossCharacter.h"
#include "Combat/AgCombatLibrary.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "Kismet/GameplayStatics.h"
#include "MotionWarpingComponent.h"

UAgBossPatternAbility::UAgBossPatternAbility()
{
	SetAssetTags(FGameplayTagContainer(AgGameplayTags::Ability_Boss_Pattern));

	ActivationBlockedTags.AddTag(AgGameplayTags::State_Groggy);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Execution_Executed);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_NonCombat);
}

FGameplayTag UAgBossPatternAbility::GetPatternTag(const FGameplayAbilitySpec& Spec)
{
	for (const FGameplayTag& Tag : Spec.GetDynamicSpecSourceTags())
	{
		if (Tag.MatchesTag(AgGameplayTags::Ability_Boss_Pattern))
		{
			return Tag;
		}
	}
	return FGameplayTag();
}

const FAgBossPattern* UAgBossPatternAbility::GetPattern() const
{
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	const FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec();
	return (Data && Spec) ? Data->FindPattern(GetPatternTag(*Spec)) : nullptr;
}

AAgBossCharacter* UAgBossPatternAbility::GetBoss() const
{
	return Cast<AAgBossCharacter>(GetAgCharacter());
}

float UAgBossPatternAbility::GetPatternPlayRate() const
{
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	const AAgBossCharacter* Boss = GetBoss();
	return (Data ? Data->PatternPlayRate : 1.f) * (Boss ? Boss->GetPatternSpeed() : 1.f);
}

bool UAgBossPatternAbility::CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	// Called on the class default object too, so everything comes from ActorInfo and the spec.
	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	const AAgCharacterBase* Boss = ActorInfo ? Cast<AAgCharacterBase>(ActorInfo->AvatarActor.Get()) : nullptr;
	const UAgBossData* Data = Boss ? Cast<UAgBossData>(Boss->GetCharacterData()) : nullptr;
	const FGameplayAbilitySpec* Spec = ASC ? ASC->FindAbilitySpecFromHandle(Handle) : nullptr;
	const FAgBossPattern* Pattern = (Data && Spec) ? Data->FindPattern(GetPatternTag(*Spec)) : nullptr;
	if (Pattern && Pattern->CooldownTag.IsValid() && ASC->HasMatchingGameplayTag(Pattern->CooldownTag))
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(Pattern->CooldownTag);
		}
		return false;
	}
	return Super::CheckCooldown(Handle, ActorInfo, OptionalRelevantTags);
}

void UAgBossPatternAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	const FAgBossPattern* Pattern = GetPattern();
	if (Pattern && Pattern->CooldownTag.IsValid() && ActorInfo)
	{
		UAgCombatLibrary::ApplyTimedTag(ActorInfo->AbilitySystemComponent.Get(), Pattern->CooldownTag, Pattern->Cooldown);
	}
}

void UAgBossPatternAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	const FAgBossPattern* Pattern = GetPattern();
	Montage = Pattern ? Data->FindPatternMontage(Pattern->Pattern) : nullptr;
	AAgCharacterBase* Boss = GetAgCharacter();
	if (!Montage || !Boss || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogAg, Verbose, TEXT("Boss pattern %s starts (play rate %.2f)"), *Pattern->Pattern.ToString(), GetPatternPlayRate());
	AActor* Player = UGameplayStatics::GetPlayerPawn(Boss, 0);
	Boss->SetActiveAttack(Pattern->Hits);
	if (UsesSharedMovement())
	{
		Boss->SetStopTarget(Player, Data->StopDistance);
	}
	if (UsesSharedMovement() && Pattern->bApproachFirstHit && Player)
	{
		// The approach may add at most the 최대 접근 거리 to the motion's own travel; the boss turns on its own (no warped facing).
		const float MaxTravel = GetForwardTravelBeforeFirstWindow(Montage) + Data->FirstHitApproachDistance;
		Boss->StartApproach(Player, Data->StopDistance, 0.f, MaxTravel, false);
	}
	else
	{
		Boss->GetMotionWarping()->RemoveWarpTarget(AAgCharacterBase::AttackWarpTarget);
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage, GetPatternPlayRate());
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageFinished);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::HandleMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->ReadyForActivation();

	// 붉은 섬광: from the lead before each unguardable hit until its attack window starts. The lead is in seconds at
	// phase 1 speed; in montage time it is the same in both phases because phase 2 shortens both by the same ratio.
	TArray<float> HitStarts;
	GetUnguardableHitStarts(HitStarts);
	const float LeadInMontageTime = Data->UnblockableFlashLead * Data->PatternPlayRate;
	for (const float HitStart : HitStarts)
	{
		UAgAbilityTask_WaitMontagePosition* FlashStart = UAgAbilityTask_WaitMontagePosition::WaitMontagePosition(this, Montage, FMath::Max(0.f, HitStart - LeadInMontageTime));
		FlashStart->OnReached.AddDynamic(this, &ThisClass::HandleFlashStart);
		FlashStart->ReadyForActivation();

		UAgAbilityTask_WaitMontagePosition* FlashEnd = UAgAbilityTask_WaitMontagePosition::WaitMontagePosition(this, Montage, HitStart);
		FlashEnd->OnReached.AddDynamic(this, &ThisClass::HandleFlashEnd);
		FlashEnd->ReadyForActivation();
	}

	OnPatternStarted();
}

void UAgBossPatternAbility::GetUnguardableHitStarts(TArray<float>& OutTimes) const
{
	const FAgBossPattern* Pattern = GetPattern();
	if (!Montage || !Pattern)
	{
		return;
	}
	for (const FAnimNotifyEvent& Notify : Montage->Notifies)
	{
		const UAgAnimNotifyState_AttackWindow* Window = Cast<UAgAnimNotifyState_AttackWindow>(Notify.NotifyStateClass);
		if (Window && Pattern->Hits.IsValidIndex(Window->HitIndex) && !Pattern->Hits[Window->HitIndex].bGuardable)
		{
			OutTimes.Add(Notify.GetTriggerTime());
		}
	}
}

float UAgBossPatternAbility::FindEventNotifyTime(const FGameplayTag& EventTag) const
{
	if (Montage)
	{
		for (const FAnimNotifyEvent& Notify : Montage->Notifies)
		{
			const UAgAnimNotify_GameplayEvent* Event = Cast<UAgAnimNotify_GameplayEvent>(Notify.Notify);
			if (Event && Event->EventTag == EventTag)
			{
				return Notify.GetTriggerTime();
			}
		}
	}
	return -1.f;
}

float UAgBossPatternAbility::GetForwardTravelBeforeFirstWindow(const UAnimMontage* InMontage)
{
	float FirstWindowStart = InMontage->GetPlayLength();
	for (const FAnimNotifyEvent& Notify : InMontage->Notifies)
	{
		if (Cast<UAgAnimNotifyState_AttackWindow>(Notify.NotifyStateClass))
		{
			FirstWindowStart = FMath::Min(FirstWindowStart, Notify.GetTriggerTime());
		}
	}
	const FTransform RootMotion = InMontage->ExtractRootMotionFromTrackRange(0.f, FirstWindowStart, FAnimExtractContext());
	return FMath::Max(0.f, RootMotion.GetTranslation().Size2D());
}

void UAgBossPatternAbility::SetFlash(bool bOn)
{
	AAgCharacterBase* Boss = GetAgCharacter();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (bFlashing == bOn || !Boss || !ASC)
	{
		return;
	}
	bFlashing = bOn;
	if (bOn)
	{
		FGameplayCueParameters Parameters;
		Parameters.Instigator = Boss;
		Parameters.EffectCauser = Boss;
		Parameters.TargetAttachComponent = Boss->GetWeaponComponent();
		ASC->AddGameplayCue(AgGameplayTags::GameplayCue_UnblockableFlash, Parameters);
	}
	else
	{
		ASC->RemoveGameplayCue(AgGameplayTags::GameplayCue_UnblockableFlash);
	}
}

void UAgBossPatternAbility::HandleFlashStart()
{
	SetFlash(true);
}

void UAgBossPatternAbility::HandleFlashEnd()
{
	SetFlash(false);
}

void UAgBossPatternAbility::HandleMontageFinished()
{
	EndSelf(false);
}

void UAgBossPatternAbility::HandleMontageCancelled()
{
	EndSelf(true);
}

void UAgBossPatternAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	SetFlash(false);
	if (AAgCharacterBase* Boss = GetAgCharacter())
	{
		Boss->ClearActiveAttack();
		Boss->ClearStopTarget();
		Boss->StopApproach();
		Boss->GetMotionWarping()->RemoveWarpTarget(AAgCharacterBase::AttackWarpTarget);
	}
	Montage = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
