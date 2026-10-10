// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_Ultimate.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Combat/AgCombatLibrary.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"

UAgAbility_Ultimate::UAgAbility_Ultimate()
{
	SetupPlayerAction(AgGameplayTags::Ability_Action_Ultimate);
}

void UAgAbility_Ultimate::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	if (!Data || !IsActive())
	{
		return;
	}

	// 보호: invincible from start to end; a motion cancel ends it too.
	bInvincible = true;
	GetAbilitySystemComponentFromActorInfo()->AddLooseGameplayTag(AgGameplayTags::State_Invincible);
	StartAttack(Data->Ultimate, Data->UltimateMontages);
}

void UAgAbility_Ultimate::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bInvincible)
	{
		bInvincible = false;
		GetAbilitySystemComponentFromActorInfo()->RemoveLooseGameplayTag(AgGameplayTags::State_Invincible);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UAgAbility_Ultimate::CanPayCost(const UAgAttributeSet& Stats) const
{
	// Usable only when UP is full.
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	return Data && Stats.GetMaxUP() > 0.f && Stats.GetUP() >= Stats.GetMaxUP() && Stats.GetUP() >= Data->UltimateUPCost;
}

FAgResourceAmounts UAgAbility_Ultimate::GetCost() const
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	return { 0.f, 0.f, Data ? Data->UltimateUPCost : 0.f };
}
