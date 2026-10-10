// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_PlayerAttack.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Character/AgPlayerCharacter.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgAttackData.h"

void UAgAbility_PlayerAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilityTask_WaitGameplayEvent* WindowTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Event_AttackWindowEnded);
	WindowTask->EventReceived.AddDynamic(this, &ThisClass::HandleAttackWindowEnded);
	WindowTask->ReadyForActivation();
}

void UAgAbility_PlayerAttack::StartAttack(const FAgPlayerAttack& Attack, TConstArrayView<TObjectPtr<UAnimMontage>> InMontages)
{
	AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(GetAgCharacter());
	Montages.Reset();
	Montages.Append(InMontages.GetData(), InMontages.Num());
	Montages.RemoveAll([](const TObjectPtr<UAnimMontage>& Montage) { return !Montage; });
	if (!Player || Montages.IsEmpty() || Attack.Hits.IsEmpty())
	{
		EndSelf(true);
		return;
	}

	bInRecovery = false;
	BeginBusy();
	HitCount = Attack.Hits.Num();
	TargetingRange = Attack.TargetingRange;
	Player->SetActiveAttack(Attack.Hits);
	Player->PrepareAttackMovement(TargetingRange);
	PlayMontageAt(0);
}

void UAgAbility_PlayerAttack::PlayMontageAt(int32 Index)
{
	// A replaced task must not end the ability when its montage stops.
	if (MontageTask)
	{
		MontageTask->EndTask();
	}
	MontageIndex = Index;
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montages[Index]);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::HandleMontageBlendingOut);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UAgAbility_PlayerAttack::HandleAttackWindowEnded(FGameplayEventData Payload)
{
	const int32 EndedHit = FMath::RoundToInt(Payload.EventMagnitude);
	if (EndedHit >= HitCount - 1)
	{
		// The recovery starts when the last attack window ends (플레이어 사양 '모션 캔슬').
		EndBusy();
		bInRecovery = true;
	}
	else if (AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(GetAgCharacter()))
	{
		// Each hit picks its target and closes in again during its own windup.
		Player->PrepareAttackMovement(TargetingRange);
	}
}

void UAgAbility_PlayerAttack::HandleMontageBlendingOut()
{
	if (Montages.IsValidIndex(MontageIndex + 1))
	{
		PlayMontageAt(MontageIndex + 1);
	}
}

void UAgAbility_PlayerAttack::HandleMontageCompleted()
{
	if (Montages.IsValidIndex(MontageIndex + 1))
	{
		PlayMontageAt(MontageIndex + 1);
		return;
	}
	MontageTask = nullptr;
	EndSelf(false);
}

void UAgAbility_PlayerAttack::HandleMontageCancelled()
{
	MontageTask = nullptr;
	EndSelf(true);
}

void UAgAbility_PlayerAttack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
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
