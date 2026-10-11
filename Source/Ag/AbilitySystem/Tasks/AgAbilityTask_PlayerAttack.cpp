// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_PlayerAttack.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/Tasks/AgAbilityTask_AttackMovement.h"
#include "AbilitySystemComponent.h"
#include "Character/AgLockOnComponent.h"
#include "Character/AgPlayerCharacter.h"
#include "Core/AgGameplayTags.h"

UAgAbilityTask_PlayerAttack* UAgAbilityTask_PlayerAttack::PlayAttack(UGameplayAbility* OwningAbility, const FAgPlayerAttack& Attack, TConstArrayView<TObjectPtr<UAnimMontage>> InMontages)
{
	UAgAbilityTask_PlayerAttack* Task = NewAbilityTask<UAgAbilityTask_PlayerAttack>(OwningAbility);
	Task->Hits = Attack.Hits;
	Task->TargetingRange = Attack.TargetingRange;
	Task->MaxApproachDistance = Attack.MaxApproachDistance;
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
	PlayMontageAt(0);

	// 몽타주를 재생하지 못하면 태스크가 그 자리에서 끝납니다. 공격 중 이동은 재생 중인 몽타주에서 접근 상한을 잽니다.
	if (IsFinished())
	{
		return;
	}
	MovementTask = UAgAbilityTask_AttackMovement::Attack(Ability, Hits.Num(), TargetingRange, MaxApproachDistance);
	MovementTask->OnHitStarted.AddDynamic(this, &ThisClass::HandleHitStarted);
	MovementTask->ReadyForActivation();
}

AAgCharacterBase* UAgAbilityTask_PlayerAttack::GetHitTarget() const
{
	return MovementTask ? MovementTask->GetTarget() : nullptr;
}

void UAgAbilityTask_PlayerAttack::HandleHitStarted()
{
	// 범위 안에 대상이 없으면 모션을 만든 그대로 재생합니다. 락온 중이면 락온 대상 쪽으로 돌고, 아니면 정면으로 갑니다.
	AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(GetAvatarActor());
	if (Player && MovementTask && !MovementTask->IsTracking())
	{
		Player->SetFacingTarget(Player->GetLockOn()->GetTarget());
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
	// 마지막 판정 구간이 끝나면 후딜이 시작됩니다(플레이어 입력 '모션 캔슬'). 타마다 다시 다가가는 것은 공격 중 이동 태스크가 합니다.
	if (FMath::RoundToInt(Payload->EventMagnitude) >= Hits.Num() - 1 && ShouldBroadcastAbilityTaskDelegates())
	{
		OnLastWindowEnded.Broadcast();
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

	// 어빌리티가 끝날 때는 어빌리티가 하위 태스크도 끝냅니다(몽타주를 멈추고, 워프 목표, 정지 간격, 회전 대상을 지움). 공격만
	// 바꿀 때(평타 다음 타)는 몽타주를 멈추지 않아, 다음 몽타주가 그 위에 블렌드해 들어옵니다.
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

	if (AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor()))
	{
		Character->ClearActiveAttack();
	}

	Super::OnDestroy(bInOwnerFinished);
}
