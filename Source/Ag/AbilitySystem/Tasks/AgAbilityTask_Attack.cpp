// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_Attack.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/Tasks/AgAbilityTask_AttackMovement.h"
#include "AbilitySystemComponent.h"
#include "Character/AgCharacterBase.h"
#include "Core/AgGameplayTags.h"

UAgAbilityTask_Attack* UAgAbilityTask_Attack::PlayAttack(UGameplayAbility* OwningAbility, const FAgPlayerAttack& Attack, UAnimMontage* InMontage)
{
	UAgAbilityTask_Attack* Task = NewAbilityTask<UAgAbilityTask_Attack>(OwningAbility);
	Task->Hits = Attack.Hits;
	Task->TargetingRange = Attack.TargetingRange;
	Task->MaxApproachDistance = Attack.MaxApproachDistance;
	Task->Montage = InMontage;
	return Task;
}

UAgAbilityTask_Attack* UAgAbilityTask_Attack::PlayAttack(UGameplayAbility* OwningAbility, const FAgBossPatternParams& Pattern, UAnimMontage* InMontage, float InPlayRate)
{
	UAgAbilityTask_Attack* Task = NewAbilityTask<UAgAbilityTask_Attack>(OwningAbility);
	Task->Hits.Reserve(Pattern.Hits.Num());
	for (const FAgAttackHit& Hit : Pattern.Hits)
	{
		Task->Hits.Emplace(Hit);
	}
	Task->MaxApproachDistance = Pattern.MaxApproachDistance;
	Task->PlayRate = InPlayRate;
	Task->Montage = InMontage;
	return Task;
}

void UAgAbilityTask_Attack::Activate()
{
	Super::Activate();

	AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor());
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (!Character || !ASC || !Montage || Hits.IsEmpty())
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnInterrupted.Broadcast();
		}
		EndTask();
		return;
	}

	WindowEndedHandle = ASC->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_AttackWindowEnded).AddUObject(this, &ThisClass::HandleAttackWindowEnded);
	RecoveryStartedHandle = ASC->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_RecoveryStarted).AddUObject(this, &ThisClass::HandleRecoveryStarted);
	Character->SetActiveAttack(Hits);
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(Ability, NAME_None, Montage, PlayRate);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->ReadyForActivation();

	// 몽타주를 재생하지 못하면 태스크가 그 자리에서 끝납니다. 공격 중 이동은 재생 중인 몽타주에서 접근 상한을 잽니다.
	if (IsFinished())
	{
		return;
	}
	MovementTask = UAgAbilityTask_AttackMovement::Attack(Ability, Hits.Num(), TargetingRange, MaxApproachDistance);
	MovementTask->ReadyForActivation();
}

AAgCharacterBase* UAgAbilityTask_Attack::GetHitTarget() const
{
	return MovementTask ? MovementTask->GetTarget() : nullptr;
}

void UAgAbilityTask_Attack::HandleAttackWindowEnded(const FGameplayEventData* Payload)
{
	// 타마다 다시 다가가는 것은 공격 중 이동 태스크가 합니다. 앞 공격 몽타주의 판정 구간 끝은 받지 않습니다.
	if (Payload->OptionalObject == Montage && FMath::RoundToInt(Payload->EventMagnitude) >= Hits.Num() - 1 && ShouldBroadcastAbilityTaskDelegates())
	{
		OnLastWindowEnded.Broadcast();
	}
}

void UAgAbilityTask_Attack::HandleRecoveryStarted(const FGameplayEventData* Payload)
{
	// 다음 타로 넘어간 뒤 블렌드 아웃 중인 앞 몽타주의 노티파이는 받지 않습니다.
	if (Payload && Payload->OptionalObject == Montage && ShouldBroadcastAbilityTaskDelegates())
	{
		OnRecoveryStarted.Broadcast();
	}
}

void UAgAbilityTask_Attack::HandleMontageCompleted()
{
	MontageTask = nullptr;
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnCompleted.Broadcast();
	}
	EndTask();
}

void UAgAbilityTask_Attack::HandleMontageCancelled()
{
	MontageTask = nullptr;
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnInterrupted.Broadcast();
	}
	EndTask();
}

void UAgAbilityTask_Attack::OnDestroy(bool bInOwnerFinished)
{
	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get(); ASC && WindowEndedHandle.IsValid())
	{
		ASC->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_AttackWindowEnded).Remove(WindowEndedHandle);
		ASC->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_RecoveryStarted).Remove(RecoveryStartedHandle);
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
