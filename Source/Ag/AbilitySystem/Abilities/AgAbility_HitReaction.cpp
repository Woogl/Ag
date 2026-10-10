// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_HitReaction.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "Character/AgCharacterBase.h"
#include "Combat/AgCombatLibrary.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"

UAgAbility_HitReaction::UAgAbility_HitReaction()
{
	bRetriggerInstancedAbility = true;

	ActivationOwnedTags.AddTag(AgGameplayTags::State_HitReaction);
	// Same priority (상태 우선순위 4): the newer reaction replaces the older one. Actions stop.
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Action);
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Reaction_KnockBack);
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Reaction_Down);

	ActivationBlockedTags.AddTag(AgGameplayTags::State_Groggy);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Execution_Executing);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Execution_Executed);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Dead);
}

void UAgAbility_HitReaction::SetupReaction(const FGameplayTag& AbilityTag, const FGameplayTag& TriggerEvent)
{
	SetAssetTags(FGameplayTagContainer(AbilityTag));

	FAbilityTriggerData& Trigger = AbilityTriggers.AddDefaulted_GetRef();
	Trigger.TriggerTag = TriggerEvent;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
}

void UAgAbility_HitReaction::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UAnimMontage* Montage = GetReactionMontage();
	AAgCharacterBase* Character = GetAgCharacter();
	if (!Montage || !Character || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Turn toward the attack so the motion pushes away from it (전투 시스템 '피격 반응').
	if (const AActor* Attacker = TriggerEventData ? TriggerEventData->Instigator.Get() : nullptr)
	{
		const FVector ToAttacker = -UAgCombatLibrary::GetAttackDirection(Attacker, Character);
		if (!ToAttacker.IsNearlyZero())
		{
			Character->SetActorRotation(FRotator(0.f, ToAttacker.Rotation().Yaw, 0.f));
		}
	}

	bPlayingFollowUp = false;
	PlayMontage(Montage);
}

void UAgAbility_HitReaction::PlayMontage(UAnimMontage* Montage)
{
	// A replaced task must not end the ability when its montage stops.
	if (MontageTask)
	{
		MontageTask->EndTask();
	}
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageFinished);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::HandleMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UAgAbility_HitReaction::HandleMontageFinished()
{
	UAnimMontage* FollowUp = bPlayingFollowUp ? nullptr : GetFollowUpMontage();
	if (FollowUp)
	{
		bPlayingFollowUp = true;
		PlayMontage(FollowUp);
		return;
	}
	MontageTask = nullptr;
	EndSelf(false);
}

void UAgAbility_HitReaction::HandleMontageCancelled()
{
	MontageTask = nullptr;
	EndSelf(true);
}

UAgAbility_KnockBack::UAgAbility_KnockBack()
{
	SetupReaction(AgGameplayTags::Ability_Reaction_KnockBack, AgGameplayTags::Event_Hit_KnockBack);
}

UAnimMontage* UAgAbility_KnockBack::GetReactionMontage() const
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	return Data ? Data->KnockBackMontage.Get() : nullptr;
}

UAgAbility_Down::UAgAbility_Down()
{
	SetupReaction(AgGameplayTags::Ability_Reaction_Down, AgGameplayTags::Event_Hit_Down);
}

UAnimMontage* UAgAbility_Down::GetReactionMontage() const
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	return Data ? Data->DownMontage.Get() : nullptr;
}

UAnimMontage* UAgAbility_Down::GetFollowUpMontage() const
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	return Data ? Data->GetUpMontage.Get() : nullptr;
}

void UAgAbility_Down::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UAbilityTask_WaitGameplayEvent* GroundedTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Event_DownGrounded, nullptr, /*OnlyTriggerOnce*/ true);
	GroundedTask->EventReceived.AddDynamic(this, &ThisClass::HandleGrounded);
	GroundedTask->ReadyForActivation();

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UAgAbility_Down::HandleGrounded(FGameplayEventData Payload)
{
	if (!bInvincible)
	{
		bInvincible = true;
		GetAbilitySystemComponentFromActorInfo()->AddLooseGameplayTag(AgGameplayTags::State_Invincible);
	}
}

void UAgAbility_Down::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bInvincible)
	{
		bInvincible = false;
		GetAbilitySystemComponentFromActorInfo()->RemoveLooseGameplayTag(AgGameplayTags::State_Invincible);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
