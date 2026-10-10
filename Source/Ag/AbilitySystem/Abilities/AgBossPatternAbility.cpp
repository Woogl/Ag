// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgBossPatternAbility.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AgAnimNotifyState_AttackWindow.h"
#include "Animation/AnimMontage.h"
#include "Character/AgCharacterBase.h"
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

float UAgBossPatternAbility::GetPatternPlayRate() const
{
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	return Data ? Data->PatternPlayRate : 1.f;
}

void UAgBossPatternAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	const FAgBossPattern* Pattern = GetPattern();
	UAnimMontage* Montage = Pattern ? Data->FindPatternMontage(Pattern->Pattern) : nullptr;
	AAgCharacterBase* Boss = GetAgCharacter();
	if (!Montage || !Boss || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* Player = UGameplayStatics::GetPlayerPawn(Boss, 0);
	Boss->SetActiveAttack(Pattern->Hits);
	Boss->SetStopTarget(Player, Data->StopDistance);
	if (Pattern->bApproachFirstHit && Player)
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
}

float UAgBossPatternAbility::GetForwardTravelBeforeFirstWindow(const UAnimMontage* Montage)
{
	float FirstWindowStart = Montage->GetPlayLength();
	for (const FAnimNotifyEvent& Notify : Montage->Notifies)
	{
		if (Cast<UAgAnimNotifyState_AttackWindow>(Notify.NotifyStateClass))
		{
			FirstWindowStart = FMath::Min(FirstWindowStart, Notify.GetTriggerTime());
		}
	}
	const FTransform RootMotion = Montage->ExtractRootMotionFromTrackRange(0.f, FirstWindowStart, FAnimExtractContext());
	return FMath::Max(0.f, RootMotion.GetTranslation().Size2D());
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
	if (AAgCharacterBase* Boss = GetAgCharacter())
	{
		Boss->ClearActiveAttack();
		Boss->ClearStopTarget();
		Boss->StopApproach();
		Boss->GetMotionWarping()->RemoveWarpTarget(AAgCharacterBase::AttackWarpTarget);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
