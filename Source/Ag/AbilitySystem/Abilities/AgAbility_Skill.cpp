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
	CostGameplayEffectClass = UAgEffect_Cost::StaticClass();
}

void UAgAbility_Skill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	const FAgSkillParams* Params = GetActionParams<FAgSkillParams>();
	if (!Params || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	BeginBusy();
	UAgAbilityTask_Attack* AttackTask = UAgAbilityTask_Attack::PlayAttack(this, *Params, Data->SkillMontage);
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

bool UAgAbility_Skill::CanPayCost(const UAgAttributeSet& Stats) const
{
	const FAgSkillParams* Params = GetActionParams<FAgSkillParams>();
	return Params && Stats.GetMP() >= Params->MPCost;
}

FAgAbilityCost UAgAbility_Skill::GetCost() const
{
	const FAgSkillParams* Params = GetActionParams<FAgSkillParams>();
	return { 0.f, Params ? Params->MPCost : 0.f, 0.f };
}
