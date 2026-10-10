// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_Guard.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "Character/AgPlayerCharacter.h"
#include "Combat/AgCombatLibrary.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"

UAgAbility_Guard::UAgAbility_Guard()
{
	// Guarding allows movement, so it doesn't own State.Acting.
	SetupPlayerAction(AgGameplayTags::Ability_Action_Guard, /*bIgnoresMovement*/ false);
}

void UAgAbility_Guard::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(GetAgCharacter());
	if (!Player || !Player->IsGuardHeld() || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilityTask_WaitGameplayEvent* PressedTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Input_Guard);
	PressedTask->EventReceived.AddDynamic(this, &ThisClass::HandleGuardPressed);
	PressedTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* ReleasedTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Input_GuardReleased);
	ReleasedTask->EventReceived.AddDynamic(this, &ThisClass::HandleGuardReleased);
	ReleasedTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* ParryEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Event_Parry);
	ParryEventTask->EventReceived.AddDynamic(this, &ThisClass::HandleParry);
	ParryEventTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* RecoveryTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Event_RecoveryStarted);
	RecoveryTask->EventReceived.AddDynamic(this, &ThisClass::HandleParryRecoveryStarted);
	RecoveryTask->ReadyForActivation();

	// 패리 구간: opened by a new guard start, not by returning to guard after a parry motion or a guard pushback.
	StartGuard(!Player->TakeGuardResume());
}

void UAgAbility_Guard::StartGuard(bool bOpenParryWindow)
{
	SetGuarding(true);
	CloseParryWindow();
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	if (bOpenParryWindow && Data)
	{
		ParryWindow = UAgCombatLibrary::ApplyTimedTag(GetAbilitySystemComponentFromActorInfo(), AgGameplayTags::State_ParryWindow, Data->ParryWindowTime);
	}
}

void UAgAbility_Guard::SetGuarding(bool bNewGuarding)
{
	if (bGuarding == bNewGuarding)
	{
		return;
	}
	bGuarding = bNewGuarding;
	if (bGuarding)
	{
		GetAbilitySystemComponentFromActorInfo()->AddLooseGameplayTag(AgGameplayTags::State_Guarding);
	}
	else
	{
		GetAbilitySystemComponentFromActorInfo()->RemoveLooseGameplayTag(AgGameplayTags::State_Guarding);
	}
}

void UAgAbility_Guard::CloseParryWindow()
{
	if (ParryWindow.IsValid())
	{
		GetAbilitySystemComponentFromActorInfo()->RemoveActiveGameplayEffect(ParryWindow);
		ParryWindow.Invalidate();
	}
}

void UAgAbility_Guard::HandleGuardPressed(FGameplayEventData Payload)
{
	if (!bInParryMotion)
	{
		return;
	}

	// During the parry motion the guard follows the key. In its recovery a new press is a new guard start.
	SetGuarding(true);
	if (bInParryRecovery)
	{
		if (ParryTask)
		{
			ParryTask->EndTask();
		}
		EndParryMotion();
		StartGuard(/*bOpenParryWindow*/ true);
	}
}

void UAgAbility_Guard::HandleGuardReleased(FGameplayEventData Payload)
{
	// Releasing during the parry motion only drops the guard; the motion plays on.
	if (bInParryMotion)
	{
		SetGuarding(false);
		return;
	}
	EndSelf(false);
}

void UAgAbility_Guard::HandleParry(FGameplayEventData Payload)
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	if (bInParryMotion || !Data || !Data->ParryMontage)
	{
		return;
	}

	// The parry motion faces where the attack came from (전투 시스템 '피격 반응').
	AAgCharacterBase* Character = GetAgCharacter();
	if (const AActor* Attacker = Payload.Instigator.Get(); Character && Attacker)
	{
		const FVector ToAttacker = -UAgCombatLibrary::GetAttackDirection(Attacker, Character);
		if (!ToAttacker.IsNearlyZero())
		{
			Character->SetActorRotation(FRotator(0.f, ToAttacker.Rotation().Yaw, 0.f));
		}
	}

	// The parry window ends once the parry motion starts. The motion is an action: no movement, no input until its recovery.
	CloseParryWindow();
	bInParryMotion = true;
	bInParryRecovery = false;
	BeginBusy();
	GetAbilitySystemComponentFromActorInfo()->AddLooseGameplayTag(AgGameplayTags::State_Acting);

	ParryTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Data->ParryMontage);
	ParryTask->OnCompleted.AddDynamic(this, &ThisClass::HandleParryMotionFinished);
	ParryTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleParryMotionCancelled);
	ParryTask->OnCancelled.AddDynamic(this, &ThisClass::HandleParryMotionCancelled);
	ParryTask->ReadyForActivation();
}

void UAgAbility_Guard::HandleParryRecoveryStarted(FGameplayEventData Payload)
{
	if (bInParryMotion)
	{
		bInParryRecovery = true;
		EndBusy();
	}
}

void UAgAbility_Guard::HandleParryMotionFinished()
{
	// Back to guard without a parry window while the key is held; otherwise back to standing.
	ParryTask = nullptr;
	EndParryMotion();
	if (!bGuarding)
	{
		EndSelf(false);
	}
}

void UAgAbility_Guard::HandleParryMotionCancelled()
{
	ParryTask = nullptr;
	EndSelf(true);
}

void UAgAbility_Guard::EndParryMotion()
{
	if (bInParryMotion)
	{
		bInParryMotion = false;
		bInParryRecovery = false;
		EndBusy();
		GetAbilitySystemComponentFromActorInfo()->RemoveLooseGameplayTag(AgGameplayTags::State_Acting);
	}
}

void UAgAbility_Guard::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	CloseParryWindow();
	EndParryMotion();
	SetGuarding(false);
	ParryTask = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
