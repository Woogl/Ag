// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_Skill.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/AgEffect_Cost.h"
#include "AbilitySystem/Tasks/AgAbilityTask_PlayerAttack.h"
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
	if (!Data || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	BeginBusy();
	UAgAbilityTask_PlayerAttack* AttackTask = UAgAbilityTask_PlayerAttack::PlayAttack(this, Data->Skill, MakeArrayView(&Data->SkillMontage, 1));
	AttackTask->OnLastWindowEnded.AddDynamic(this, &ThisClass::HandleLastWindowEnded);
	AttackTask->OnCompleted.AddDynamic(this, &ThisClass::HandleAttackCompleted);
	AttackTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleAttackInterrupted);
	AttackTask->ReadyForActivation();
}

void UAgAbility_Skill::HandleLastWindowEnded()
{
	// 마지막 판정 구간이 끝나면 후딜이 시작됩니다.
	EndBusy();
}

void UAgAbility_Skill::HandleAttackCompleted()
{
	EndSelf(false);
}

void UAgAbility_Skill::HandleAttackInterrupted()
{
	EndSelf(true);
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

FAgAbilityCost UAgAbility_Skill::GetCost() const
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	return { 0.f, Data ? Data->SkillMPCost : 0.f, 0.f };
}
