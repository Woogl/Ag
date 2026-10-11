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
	if (!Data->GuardPushbackMontage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 공격 쪽으로 돌아, 모션이 공격 반대쪽으로 밀려나게 합니다(전투 시스템 '피격 반응').
	if (const AActor* Attacker = TriggerEventData ? TriggerEventData->Instigator.Get() : nullptr)
	{
		const FVector ToAttacker = -UAgCombatLibrary::GetHorizontalDirection(Attacker, Character);
		if (!ToAttacker.IsNearlyZero())
		{
			Character->SetActorRotation(FRotator(0.f, ToAttacker.Rotation().Yaw, 0.f));
		}
	}

	UAgAbilityTask_PlayReaction* ReactionTask = UAgAbilityTask_PlayReaction::PlayReaction(this, Data->GuardPushbackMontage);
	ReactionTask->OnCompleted.AddDynamic(this, &ThisClass::HandleReactionCompleted);
	ReactionTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleReactionInterrupted);
	ReactionTask->ReadyForActivation();
}

void UAgAbility_GuardPushback::HandleReactionCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UAgAbility_GuardPushback::HandleReactionInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
