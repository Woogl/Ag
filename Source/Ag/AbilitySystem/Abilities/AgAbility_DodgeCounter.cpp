// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_DodgeCounter.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"

UAgAbility_DodgeCounter::UAgAbility_DodgeCounter()
{
	SetupPlayerAction(AgGameplayTags::Ability_Action_DodgeCounter);
	ActivationRequiredTags.AddTag(AgGameplayTags::State_DodgeCounterChance);
}

void UAgAbility_DodgeCounter::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	if (!Data || !IsActive())
	{
		return;
	}

	// The chance is used up.
	GetAbilitySystemComponentFromActorInfo()->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(AgGameplayTags::State_DodgeCounterChance));

	SetInvincible(true);
	UAbilityTask_WaitGameplayEvent* WindowTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Event_AttackWindowEnded);
	WindowTask->EventReceived.AddDynamic(this, &ThisClass::HandleInvincibleWindowEnded);
	WindowTask->ReadyForActivation();

	StartAttack(Data->DodgeCounter, MakeArrayView(&Data->DodgeCounterMontage, 1));
}

void UAgAbility_DodgeCounter::HandleInvincibleWindowEnded(FGameplayEventData Payload)
{
	SetInvincible(false);
}

void UAgAbility_DodgeCounter::SetInvincible(bool bNewInvincible)
{
	if (bInvincible == bNewInvincible)
	{
		return;
	}
	bInvincible = bNewInvincible;
	if (bInvincible)
	{
		GetAbilitySystemComponentFromActorInfo()->AddLooseGameplayTag(AgGameplayTags::State_Invincible);
	}
	else
	{
		GetAbilitySystemComponentFromActorInfo()->RemoveLooseGameplayTag(AgGameplayTags::State_Invincible);
	}
}

void UAgAbility_DodgeCounter::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	SetInvincible(false);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
