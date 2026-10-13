// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_Skill.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/AgEffect_Cost.h"
#include "AbilitySystem/Tasks/AgAbilityTask_Attack.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"

UAgAbility_Skill::UAgAbility_Skill()
{
	SetupPlayerAction(AgGameplayTags::Ability_Action_Skill);
	SetCooldownTag(AgGameplayTags::Cooldown_Skill);
	CostGameplayEffectClass = UAgEffect_Cost::StaticClass();
}

void UAgAbility_Skill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	BeginBusy();
	UAgAbilityTask_Attack* AttackTask = UAgAbilityTask_Attack::PlayAttack(this, Data->Skill, Data->SkillMontage);
	AttackTask->OnRecoveryStarted.AddDynamic(this, &ThisClass::HandleRecoveryStarted);
	AttackTask->OnCompleted.AddDynamic(this, &ThisClass::HandleAttackCompleted);
	AttackTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleAttackInterrupted);
	AttackTask->ReadyForActivation();
}

void UAgAbility_Skill::HandleRecoveryStarted()
{
	EndBusy();
}

void UAgAbility_Skill::HandleAttackCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UAgAbility_Skill::HandleAttackInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

float UAgAbility_Skill::GetCooldownDuration() const
{
	return GetCharacterData<UAgPlayerData>()->SkillCooldown;
}

bool UAgAbility_Skill::CanPayCost(const UAgAttributeSet& Stats) const
{
	return Stats.GetMP() >= GetCharacterData<UAgPlayerData>()->SkillMPCost;
}

FAgAbilityCost UAgAbility_Skill::GetCost() const
{
	return { 0.f, GetCharacterData<UAgPlayerData>()->SkillMPCost, 0.f };
}
