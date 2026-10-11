// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_PlayerAttack.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/Tasks/AgAbilityTask_AttackMovement.h"
#include "AbilitySystemComponent.h"
#include "Character/AgLockOnComponent.h"
#include "Character/AgPlayerCharacter.h"
#include "Combat/AgCombatLibrary.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"

UAgAbilityTask_PlayerAttack* UAgAbilityTask_PlayerAttack::PlayAttack(UGameplayAbility* OwningAbility, const FAgPlayerAttack& Attack, TConstArrayView<TObjectPtr<UAnimMontage>> InMontages)
{
	UAgAbilityTask_PlayerAttack* Task = NewAbilityTask<UAgAbilityTask_PlayerAttack>(OwningAbility);
	Task->Hits = Attack.Hits;
	Task->TargetingRange = Attack.TargetingRange;
	for (const TObjectPtr<UAnimMontage>& Montage : InMontages)
	{
		if (Montage)
		{
			Task->Montages.Add(Montage);
		}
	}
	return Task;
}

void UAgAbilityTask_PlayerAttack::Activate()
{
	Super::Activate();

	AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(GetAvatarActor());
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (!Player || !ASC || Montages.IsEmpty() || Hits.IsEmpty())
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnInterrupted.Broadcast();
		}
		EndTask();
		return;
	}

	WindowEndedHandle = ASC->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_AttackWindowEnded).AddUObject(this, &ThisClass::HandleAttackWindowEnded);
	Player->SetActiveAttack(Hits);
	StartHitMovement();
	PlayMontageAt(0);
}

void UAgAbilityTask_PlayerAttack::StartHitMovement()
{
	// 이전 타의 이동 태스크를 먼저 끝내야, 그 정리가 새 타의 워프 목표와 정지 간격을 지우지 않습니다.
	if (MovementTask)
	{
		MovementTask->EndTask();
		MovementTask = nullptr;
	}
	HitTarget.Reset();

	AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(GetAvatarActor());
	const UAgPlayerData* Data = Player ? Cast<UAgPlayerData>(Player->GetCharacterData()) : nullptr;
	if (!Data)
	{
		return;
	}

	// 정지 간격은 가장 가까운 적이 타겟팅 범위 안에 있든 없든 지킵니다. 다가가는 거리는 타를 시작할 때의 접근 목표까지라,
	// 선딜 중 대상이 물러나면(B1) 끝까지 쫓지 않고 그만큼 간격이 벌어집니다.
	AAgCharacterBase* Enemy = Player->FindEnemy(TNumericLimits<float>::Max());
	HitTarget = Enemy;
	const float Gap = UAgCombatLibrary::GetHorizontalDistance(Player, Enemy);
	if (Enemy)
	{
		const float MaxTravel = FMath::Max(0.f, Gap - Data->AttackStopDistance);
		MovementTask = UAgAbilityTask_AttackMovement::KeepGap(Ability, Enemy, Data->AttackStopDistance, MaxTravel, TargetingRange);
		MovementTask->ReadyForActivation();
	}

	// 범위 안에 대상이 없으면 모션을 만든 그대로 재생합니다. 락온 중이면 락온 대상 쪽, 아니면 정면으로 갑니다.
	const AActor* LockOnTarget = Player->GetLockOn()->GetTarget();
	if (Gap > TargetingRange && LockOnTarget)
	{
		const FVector ToTarget = UAgCombatLibrary::GetHorizontalDirection(Player, LockOnTarget);
		if (!ToTarget.IsNearlyZero())
		{
			Player->SetActorRotation(FRotator(0.f, ToTarget.Rotation().Yaw, 0.f));
		}
	}
}

void UAgAbilityTask_PlayerAttack::PlayMontageAt(int32 Index)
{
	// 바꾼 몽타주 태스크는 그 몽타주가 멈춰도 공격을 끝내지 않습니다.
	if (MontageTask)
	{
		MontageTask->EndTask();
	}
	MontageIndex = Index;
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(Ability, NAME_None, Montages[Index]);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::HandleMontageBlendingOut);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UAgAbilityTask_PlayerAttack::HandleAttackWindowEnded(const FGameplayEventData* Payload)
{
	const int32 EndedHit = FMath::RoundToInt(Payload->EventMagnitude);
	if (EndedHit >= Hits.Num() - 1)
	{
		// 마지막 판정 구간이 끝나면 후딜이 시작됩니다(플레이어 입력 '모션 캔슬').
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnLastWindowEnded.Broadcast();
		}
	}
	else
	{
		// 타마다 대상을 다시 고르고, 그 타의 선딜 동안 다시 다가갑니다.
		StartHitMovement();
	}
}

void UAgAbilityTask_PlayerAttack::HandleMontageBlendingOut()
{
	if (Montages.IsValidIndex(MontageIndex + 1))
	{
		PlayMontageAt(MontageIndex + 1);
	}
}

void UAgAbilityTask_PlayerAttack::HandleMontageCompleted()
{
	MontageTask = nullptr;
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnCompleted.Broadcast();
	}
	EndTask();
}

void UAgAbilityTask_PlayerAttack::HandleMontageCancelled()
{
	MontageTask = nullptr;
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnInterrupted.Broadcast();
	}
	EndTask();
}

void UAgAbilityTask_PlayerAttack::OnDestroy(bool bInOwnerFinished)
{
	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get(); ASC && WindowEndedHandle.IsValid())
	{
		ASC->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_AttackWindowEnded).Remove(WindowEndedHandle);
	}

	// 어빌리티가 끝날 때는 어빌리티가 하위 태스크도 끝냅니다(몽타주를 멈추고, 워프 목표와 정지 간격을 지움). 공격만 바꿀 때(평타
	// 다음 타)는 몽타주를 멈추지 않아, 다음 몽타주가 그 위에 블렌드해 들어옵니다.
	if (!bInOwnerFinished)
	{
		if (MontageTask)
		{
			MontageTask->EndTask();
		}
		if (MovementTask)
		{
			MovementTask->EndTask();
		}
	}
	MontageTask = nullptr;
	MovementTask = nullptr;
	HitTarget.Reset();

	if (AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor()))
	{
		Character->ClearActiveAttack();
	}

	Super::OnDestroy(bInOwnerFinished);
}
