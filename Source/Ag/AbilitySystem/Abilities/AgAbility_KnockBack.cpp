// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_KnockBack.h"

#include "AbilitySystem/Tasks/AgAbilityTask_PlayReaction.h"
#include "Character/AgCharacterBase.h"
#include "Combat/AgCombatLibrary.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"

UAgAbility_KnockBack::UAgAbility_KnockBack()
{
	SetupHitReaction(AgGameplayTags::Ability_Reaction_HitReaction_KnockBack, AgGameplayTags::Event_Hit_KnockBack);
}

void UAgAbility_KnockBack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const UAgCharacterData* Data = GetCharacterData<UAgCharacterData>();
	AAgCharacterBase* Character = GetAgCharacter();
	if (!Data->KnockBackMontage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
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

	UAgAbilityTask_PlayReaction* ReactionTask = UAgAbilityTask_PlayReaction::PlayReaction(this, Data->KnockBackMontage);
	ReactionTask->OnCompleted.AddDynamic(this, &ThisClass::HandleReactionCompleted);
	ReactionTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleReactionInterrupted);
	ReactionTask->ReadyForActivation();
}

void UAgAbility_KnockBack::HandleReactionCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UAgAbility_KnockBack::HandleReactionInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
