// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_Skill.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "Combat/AgCombatLibrary.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"

UAgAbility_Skill::UAgAbility_Skill()
{
	SetupPlayerAction(AgGameplayTags::Ability_Action_Skill);
	SetCooldownTag(AgGameplayTags::Cooldown_Skill);
}

void UAgAbility_Skill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>(); Data && IsActive())
	{
		StartAttack(Data->Skill, MakeArrayView(&Data->SkillMontage, 1));
	}
}

float UAgAbility_Skill::GetCooldownDuration() const
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	return Data ? Data->SkillCooldown : 0.f;
}

bool UAgAbility_Skill::CanPayCost(const UAgAttributeSet& Stats) const
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	return Data && Stats.GetMP() >= Data->SkillMPCost;
}

FAgResourceAmounts UAgAbility_Skill::GetCost() const
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	return { 0.f, Data ? Data->SkillMPCost : 0.f, 0.f };
}
