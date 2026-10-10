// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_BasicAttack.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Ag.h"
#include "Character/AgPlayerCharacter.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"

UAgAbility_BasicAttack::UAgAbility_BasicAttack()
{
	SetupPlayerAction(AgGameplayTags::Ability_Action_BasicAttack);
}

void UAgAbility_BasicAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilityTask_WaitGameplayEvent* InputTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Input_Attack);
	InputTask->EventReceived.AddDynamic(this, &ThisClass::HandleAttackInput);
	InputTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* WindowTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Event_AttackWindowEnded);
	WindowTask->EventReceived.AddDynamic(this, &ThisClass::HandleAttackWindowEnded);
	WindowTask->ReadyForActivation();

	StartHit(0);
}

void UAgAbility_BasicAttack::StartHit(int32 Index)
{
	AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(GetAgCharacter());
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	UAnimMontage* Montage = (Data && Data->BasicAttackMontages.IsValidIndex(Index)) ? Data->BasicAttackMontages[Index].Get() : nullptr;
	if (!Player || !Data || !Data->BasicAttack.IsValidIndex(Index) || !Montage)
	{
		EndSelf(true);
		return;
	}

	HitIndex = Index;
	bInRecovery = false;
	BeginBusy();
	UE_LOG(LogAg, Verbose, TEXT("Basic attack: hit %d starts"), Index + 1);

	const FAgPlayerAttack& Attack = Data->BasicAttack[Index];
	Player->SetActiveAttack(Attack.Hits);
	Player->PrepareAttackMovement(Attack.TargetingRange);

	// The previous hit's task must not end the ability when its montage is replaced.
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

void UAgAbility_BasicAttack::HandleAttackInput(FGameplayEventData Payload)
{
	// 평타 키의 처리 순서: during a hit's recovery the next hit starts; before it, the input is ignored.
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	if (bInRecovery && Data && Data->BasicAttack.Num() > 0)
	{
		StartHit((HitIndex + 1) % Data->BasicAttack.Num());
	}
}

void UAgAbility_BasicAttack::HandleAttackWindowEnded(FGameplayEventData Payload)
{
	// The recovery starts when the hit's last attack window ends.
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	if (Data && Data->BasicAttack.IsValidIndex(HitIndex) && FMath::RoundToInt(Payload.EventMagnitude) == Data->BasicAttack[HitIndex].Hits.Num() - 1)
	{
		EndBusy();
		bInRecovery = true;
	}
}

void UAgAbility_BasicAttack::HandleMontageFinished()
{
	EndSelf(false);
}

void UAgAbility_BasicAttack::HandleMontageCancelled()
{
	EndSelf(true);
}

void UAgAbility_BasicAttack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	bInRecovery = false;
	MontageTask = nullptr;
	if (AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(GetAgCharacter()))
	{
		Player->ClearActiveAttack();
		Player->ClearAttackMovement();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
