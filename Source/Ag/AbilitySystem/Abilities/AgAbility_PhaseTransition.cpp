// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_PhaseTransition.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/AgAttributeSet.h"
#include "Ag.h"
#include "Character/AgBossCharacter.h"
#include "Combat/AgCombatLibrary.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"

UAgAbility_PhaseTransition::UAgAbility_PhaseTransition()
{
	SetAssetTags(FGameplayTagContainer(AgGameplayTags::Ability_Reaction_PhaseTransition));
	ActivationOwnedTags.AddTag(AgGameplayTags::State_Invincible);
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Boss_Pattern);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Groggy);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Dead);

	FAbilityTriggerData& Trigger = AbilityTriggers.AddDefaulted_GetRef();
	Trigger.TriggerTag = AgGameplayTags::Event_PhaseTransition;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
}

void UAgAbility_PhaseTransition::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	if (!Data || !Data->PhaseTransitionMontage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogAg, Verbose, TEXT("Boss phase transition starts"));

	// 1. Invincible (owned tag) while the roar plays.
	UAbilityTask_PlayMontageAndWait* RoarTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Data->PhaseTransitionMontage);
	RoarTask->OnCompleted.AddDynamic(this, &ThisClass::HandleRoarFinished);
	RoarTask->OnBlendOut.AddDynamic(this, &ThisClass::HandleRoarFinished);
	RoarTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleRoarCancelled);
	RoarTask->OnCancelled.AddDynamic(this, &ThisClass::HandleRoarCancelled);
	RoarTask->ReadyForActivation();
}

void UAgAbility_PhaseTransition::HandleRoarFinished()
{
	// 2. PP refills, and phase 2 starts once the transition ends. (3. The AI waits and picks a phase 2 pattern.)
	if (AAgBossCharacter* Boss = Cast<AAgBossCharacter>(GetAgCharacter()))
	{
		const UAgAttributeSet* Stats = Boss->GetAttributeSet();
		UAgCombatLibrary::ApplyStatChange(Boss, Boss, 0.f, Stats->GetMaxPP() - Stats->GetPP());
		Boss->EnterPhase2();
	}
	EndSelf(false);
}

void UAgAbility_PhaseTransition::HandleRoarCancelled()
{
	EndSelf(true);
}
