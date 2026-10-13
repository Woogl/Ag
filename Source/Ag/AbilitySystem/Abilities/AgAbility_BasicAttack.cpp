// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_BasicAttack.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/Tasks/AgAbilityTask_Attack.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Ag.h"
#include "Character/AgCharacterBase.h"
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

	StartHit(0);
}

void UAgAbility_BasicAttack::StartHit(int32 Index)
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	if (!Data->BasicAttack.IsValidIndex(Index) || !Data->BasicAttackMontages.IsValidIndex(Index))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	HitIndex = Index;
	UE_LOG(LogAg, Verbose, TEXT("Basic attack: hit %d starts"), Index + 1);
	if (AttackTask)
	{
		AttackTask->EndTask();
	}
	BeginBusy();
	AttackTask = UAgAbilityTask_Attack::PlayAttack(this, Data->BasicAttack[Index], Data->BasicAttackMontages[Index]);
	AttackTask->OnRecoveryStarted.AddDynamic(this, &ThisClass::HandleRecoveryStarted);
	AttackTask->OnCompleted.AddDynamic(this, &ThisClass::HandleAttackCompleted);
	AttackTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleAttackInterrupted);
	AttackTask->ReadyForActivation();

	// 평타의 각 타를 시작할 때마다, 그 타의 대상인 보스가 Backstep을 검사합니다.
	FGameplayEventData Payload;
	Payload.EventTag = AgGameplayTags::Event_BasicAttackStarted;
	Payload.Instigator = GetAvatarActorFromActorInfo();
	Payload.Target = AttackTask->GetHitTarget();
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetAvatarActorFromActorInfo(), AgGameplayTags::Event_BasicAttackStarted, Payload);
}

void UAgAbility_BasicAttack::HandleAttackInput(FGameplayEventData Payload)
{
	// 평타 키의 처리 순서: 후딜 중이면(행동 중이 풀림) 다음 타를 시작하고, 그 전이면 무시합니다.
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	if (!IsBusy() && Data->BasicAttack.Num() > 0)
	{
		StartHit((HitIndex + 1) % Data->BasicAttack.Num());
	}
}

void UAgAbility_BasicAttack::HandleRecoveryStarted()
{
	// 후딜이 시작되면 다음 타와 다른 행동을 받습니다.
	EndBusy();
}

void UAgAbility_BasicAttack::HandleAttackCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UAgAbility_BasicAttack::HandleAttackInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
