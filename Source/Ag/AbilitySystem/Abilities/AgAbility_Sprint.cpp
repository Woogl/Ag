// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_Sprint.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "Core/AgGameplayTags.h"

UAgAbility_Sprint::UAgAbility_Sprint()
{
	// Movement stays with the player; only the speed and facing change.
	SetupPlayerAction(AgGameplayTags::Ability_Action_Sprint, /*bIgnoresMovement*/ false);
	ActivationOwnedTags.AddTag(AgGameplayTags::State_Sprinting);
}

void UAgAbility_Sprint::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

bool UAgAbility_Sprint::CanPayCost(const UAgAttributeSet& Stats) const
{
	// Usable while SP is above 0 (플레이어 사양 'SP 소모').
	return Stats.GetSP() > 0.f;
}
