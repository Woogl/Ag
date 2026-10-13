// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_GuardBreak.h"

#include "AbilitySystem/Tasks/AgAbilityTask_PlayReaction.h"
#include "Character/AgCharacterBase.h"
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
	if (!Data->GuardBreakMontage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 공격 쪽으로 돌아, 모션이 공격 반대쪽으로 밀려나게 합니다(전투 시스템 '피격 반응').
	if (const AActor* Attacker = TriggerEventData ? TriggerEventData->Instigator.Get() : nullptr)
	{
		const FVector ToAttacker = (Attacker->GetActorLocation() - Character->GetActorLocation()).GetSafeNormal2D();
		if (!ToAttacker.IsNearlyZero())
		{
			Character->SetActorRotation(FRotator(0.f, ToAttacker.Rotation().Yaw, 0.f));
		}
	}

	UAgAbilityTask_PlayReaction* ReactionTask = UAgAbilityTask_PlayReaction::PlayReaction(this, Data->GuardBreakMontage);
	ReactionTask->OnCompleted.AddDynamic(this, &ThisClass::HandleReactionCompleted);
	ReactionTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleReactionInterrupted);
	ReactionTask->ReadyForActivation();
}

void UAgAbility_GuardBreak::HandleReactionCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UAgAbility_GuardBreak::HandleReactionInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
