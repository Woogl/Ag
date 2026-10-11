// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_GuardPushback.h"

#include "AbilitySystem/Tasks/AgAbilityTask_PlayReaction.h"
#include "Character/AgCharacterBase.h"
#include "Combat/AgCombatLibrary.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"

UAgAbility_GuardPushback::UAgAbility_GuardPushback()
{
	SetupHitReaction(AgGameplayTags::Ability_Reaction_HitReaction_GuardPushback, AgGameplayTags::Event_Guard_Pushback);
	ActivationOwnedTags.AddTag(AgGameplayTags::State_Guarding);
}

void UAgAbility_GuardPushback::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	AAgCharacterBase* Character = GetAgCharacter();
	if (!Data || !Data->GuardPushbackMontage || !Character || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 공격 쪽으로 돌아, 모션이 공격 반대쪽으로 밀려나게 합니다(전투 시스템 '피격 반응').
	UAgCombatLibrary::TurnToward(Character, TriggerEventData ? TriggerEventData->Instigator.Get() : nullptr);

	UAgAbilityTask_PlayReaction* ReactionTask = UAgAbilityTask_PlayReaction::PlayReaction(this, Data->GuardPushbackMontage);
	ReactionTask->OnCompleted.AddDynamic(this, &ThisClass::HandleReactionCompleted);
	ReactionTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleReactionInterrupted);
	ReactionTask->ReadyForActivation();
}

void UAgAbility_GuardPushback::HandleReactionCompleted()
{
	EndSelf(false);
}

void UAgAbility_GuardPushback::HandleReactionInterrupted()
{
	EndSelf(true);
}
