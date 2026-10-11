// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_GuardBreak.h"

#include "AbilitySystem/Tasks/AgAbilityTask_PlayReaction.h"
#include "Character/AgCharacterBase.h"
#include "Combat/AgCombatLibrary.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"

UAgAbility_GuardBreak::UAgAbility_GuardBreak()
{
	SetupHitReaction(AgGameplayTags::Ability_Reaction_HitReaction_GuardBreak, AgGameplayTags::Event_Guard_Break);
}

void UAgAbility_GuardBreak::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	AAgCharacterBase* Character = GetAgCharacter();
	if (!Data || !Data->GuardBreakMontage || !Character || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 공격 쪽으로 돌아, 모션이 공격 반대쪽으로 밀려나게 합니다(전투 시스템 '피격 반응').
	UAgCombatLibrary::TurnToward(Character, TriggerEventData ? TriggerEventData->Instigator.Get() : nullptr);

	UAgAbilityTask_PlayReaction* ReactionTask = UAgAbilityTask_PlayReaction::PlayReaction(this, Data->GuardBreakMontage);
	ReactionTask->OnCompleted.AddDynamic(this, &ThisClass::HandleReactionCompleted);
	ReactionTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleReactionInterrupted);
	ReactionTask->ReadyForActivation();
}

void UAgAbility_GuardBreak::HandleReactionCompleted()
{
	EndSelf(false);
}

void UAgAbility_GuardBreak::HandleReactionInterrupted()
{
	EndSelf(true);
}
