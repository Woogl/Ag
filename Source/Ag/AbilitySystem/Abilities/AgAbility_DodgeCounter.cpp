// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_DodgeCounter.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/Tasks/AgAbilityTask_HoldTag.h"
#include "AbilitySystem/Tasks/AgAbilityTask_PlayerAttack.h"
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
	if (!Data || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 시작부터 판정 구간이 끝날 때까지 무적입니다.
	InvincibleTag = UAgAbilityTask_HoldTag::HoldTag(this, AgGameplayTags::State_Invincible);
	InvincibleTag->ReadyForActivation();
	UAbilityTask_WaitGameplayEvent* WindowTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Event_AttackWindowEnded);
	WindowTask->EventReceived.AddDynamic(this, &ThisClass::HandleInvincibleWindowEnded);
	WindowTask->ReadyForActivation();

	BeginBusy();
	UAgAbilityTask_PlayerAttack* AttackTask = UAgAbilityTask_PlayerAttack::PlayAttack(this, Data->DodgeCounter, MakeArrayView(&Data->DodgeCounterMontage, 1));
	AttackTask->OnLastWindowEnded.AddDynamic(this, &ThisClass::HandleLastWindowEnded);
	AttackTask->OnCompleted.AddDynamic(this, &ThisClass::HandleAttackCompleted);
	AttackTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleAttackInterrupted);
	AttackTask->ReadyForActivation();
}

void UAgAbility_DodgeCounter::HandleInvincibleWindowEnded(FGameplayEventData Payload)
{
	InvincibleTag->EndTask();
}

void UAgAbility_DodgeCounter::HandleLastWindowEnded()
{
	// 마지막 판정 구간이 끝나면 후딜이 시작됩니다.
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

