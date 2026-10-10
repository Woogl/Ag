// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_Death.h"

#include "Character/AgCharacterBase.h"
#include "Combat/AgCombatLibrary.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCombatRules.h"

UAgAbility_Death::UAgAbility_Death()
{
	SetAssetTags(FGameplayTagContainer(AgGameplayTags::Ability_Reaction_Death));
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Dead);

	FAbilityTriggerData& Trigger = AbilityTriggers.AddDefaulted_GetRef();
	Trigger.TriggerTag = AgGameplayTags::Event_Death;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
}

void UAgAbility_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (AAgCharacterBase* Character = GetAgCharacter())
	{
		// Pushed the way the last attack came from.
		const AActor* Attacker = TriggerEventData ? TriggerEventData->Instigator.Get() : nullptr;
		const UAgCombatRules* Rules = UAgCombatLibrary::GetCombatRules();
		const FVector Push = UAgCombatLibrary::GetAttackDirection(Attacker, Character) * (Rules ? Rules->DeathPushSpeed : 0.f);
		Character->Die(Push, this);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
