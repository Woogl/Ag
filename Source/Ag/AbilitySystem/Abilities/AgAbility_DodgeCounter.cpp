// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_DodgeCounter.h"

#include "AbilitySystem/Tasks/AgAbilityTask_Attack.h"
#include "AbilitySystem/Tasks/AgAbilityTask_HoldTag.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"

UAgAbility_DodgeCounter::UAgAbility_DodgeCounter()
{
	SetupPlayerAction(AgGameplayTags::Ability_Action_DodgeCounter);
	ActivationRequiredTags.AddTag(AgGameplayTags::State_DodgeCounterChance);
}

void UAgAbility_DodgeCounter::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	const FAgPlayerAttack* Params = GetAbilityParams<FAgPlayerAttack>();
	if (!Params || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 시작부터 판정 구간이 끝날 때까지 무적입니다.
	InvincibleTag = UAgAbilityTask_HoldTag::HoldTag(this, AgGameplayTags::State_Invincible);
	InvincibleTag->ReadyForActivation();

	BeginBusy();
	UAgAbilityTask_Attack* AttackTask = UAgAbilityTask_Attack::PlayAttack(this, *Params, Data->FindMontage(GetAbilityId()));
	AttackTask->OnLastWindowEnded.AddDynamic(this, &ThisClass::HandleLastWindowEnded);
	AttackTask->OnRecoveryStarted.AddDynamic(this, &ThisClass::HandleRecoveryStarted);
	AttackTask->OnCompleted.AddDynamic(this, &ThisClass::HandleAttackCompleted);
	AttackTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleAttackInterrupted);
	AttackTask->ReadyForActivation();
}

void UAgAbility_DodgeCounter::HandleLastWindowEnded()
{
	// 무적은 판정 구간이 끝날 때까지입니다.
	InvincibleTag->EndTask();
}

void UAgAbility_DodgeCounter::HandleRecoveryStarted()
{
	EndBusy();
}

void UAgAbility_DodgeCounter::HandleAttackCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UAgAbility_DodgeCounter::HandleAttackInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

