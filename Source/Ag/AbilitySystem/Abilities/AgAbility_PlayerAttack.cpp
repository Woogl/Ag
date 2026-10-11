// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_PlayerAttack.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/Tasks/AgAbilityTask_AttackMovement.h"
#include "Character/AgBossCharacter.h"
#include "Character/AgLockOnComponent.h"
#include "Character/AgPlayerCharacter.h"
#include "Combat/AgCombatLibrary.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgAttackData.h"
#include "Data/AgCharacterData.h"

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
	StartHitMovement();
	PlayMontageAt(0);
}

void UAgAbility_PlayerAttack::StartHitMovement()
{
	// 이전 타의 태스크를 먼저 끝내야, 그 정리가 새 타의 워프 목표와 정지 간격을 지우지 않습니다.
	if (MovementTask)
	{
		MovementTask->EndTask();
		MovementTask = nullptr;
	}

	AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(GetAgCharacter());
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	if (!Player || !Data)
	{
		return;
	}

	// 정지 간격은 보스가 타겟팅 범위 안에 있든 없든 지킵니다. 다가가는 거리는 타를 시작할 때의 접근 목표까지라,
	// 선딜 중 보스가 물러나면(B1) 끝까지 쫓지 않고 그만큼 간격이 벌어집니다.
	AAgBossCharacter* Boss = Player->FindBoss(TNumericLimits<float>::Max());
	const float Gap = UAgCombatLibrary::GetHorizontalDistance(Player, Boss);
	if (Boss)
	{
		const float MaxTravel = FMath::Max(0.f, Gap - Data->AttackStopDistance);
		MovementTask = UAgAbilityTask_AttackMovement::KeepGap(this, Boss, Data->AttackStopDistance, MaxTravel, TargetingRange);
		MovementTask->ReadyForActivation();
	}

	// 범위 안에 대상이 없으면 모션을 만든 그대로 재생합니다. 락온 중이면 락온 대상 쪽, 아니면 정면으로 갑니다.
	const AActor* LockOnTarget = Player->GetLockOn()->GetTarget();
	if (Gap > TargetingRange && LockOnTarget)
	{
		const FVector ToTarget = UAgCombatLibrary::GetAttackDirection(Player, LockOnTarget);
		if (!ToTarget.IsNearlyZero())
		{
			Player->SetActorRotation(FRotator(0.f, ToTarget.Rotation().Yaw, 0.f));
		}
	}
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
		// 마지막 판정 구간이 끝나면 후딜이 시작됩니다(플레이어 입력 '모션 캔슬').
		EndBusy();
		bInRecovery = true;
	}
	else
	{
		// 타마다 대상을 다시 고르고, 그 타의 선딜 동안 다시 다가갑니다.
		StartHitMovement();
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
	MovementTask = nullptr;
	if (AAgCharacterBase* Character = GetAgCharacter())
	{
		Character->ClearActiveAttack();
	}

	// 공격 중 이동 태스크는 어빌리티와 함께 끝나며 워프 목표와 정지 간격을 지웁니다.
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
