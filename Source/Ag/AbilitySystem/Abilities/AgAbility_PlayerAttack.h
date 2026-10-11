// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_PlayerAttack.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAgAbilityTask_AttackMovement;
class UAnimMontage;
struct FAgPlayerAttack;

/**
 * 플레이어 공격 행동(평타 한 타, 회피 반격, 스킬, 궁극기): 몽타주를 공격 데이터와 함께 이어 재생하고, 타마다 공격 방향과
 * 공격 중 이동을 정하며(플레이어 사양 '공격 방향', '공격 중 이동'), 마지막 판정 구간이 끝나면 후딜을 시작합니다.
 * 자식 클래스는 ActivateAbility에서 커밋한 뒤 StartAttack을 부릅니다.
 */
UCLASS(Abstract)
class UAgAbility_PlayerAttack : public UAgGameplayAbility
{
	GENERATED_BODY()

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** Starts (or restarts) the attack. Montages play back to back, each blended into the next. Ends the ability if there is nothing to play. */
	void StartAttack(const FAgPlayerAttack& Attack, TConstArrayView<TObjectPtr<UAnimMontage>> InMontages);

	bool IsInRecovery() const { return bInRecovery; }

private:
	void PlayMontageAt(int32 Index);

	/**
	 * 한 타의 공격 방향과 공격 중 이동: 보스와 정지 간격을 지키고, 보스가 이 타의 타겟팅 범위 안이면 선딜 동안 다가가며 그쪽을
	 * 바라봅니다. 범위 안에 보스가 없으면 락온 중일 때만 락온 대상 쪽으로 돕니다.
	 */
	void StartHitMovement();

	UFUNCTION()
	void HandleAttackWindowEnded(FGameplayEventData Payload);

	/** A chained montage starts as the previous one blends out (chained montages blend out from their very end). */
	UFUNCTION()
	void HandleMontageBlendingOut();

	/** The motion has ended: the recovery (후딜) lasts to here, through the blend-out. */
	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageCancelled();

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	UPROPERTY()
	TObjectPtr<UAgAbilityTask_AttackMovement> MovementTask;

	UPROPERTY()
	TArray<TObjectPtr<UAnimMontage>> Montages;

	int32 MontageIndex = 0;
	int32 HitCount = 0;
	float TargetingRange = 0.f;
	bool bInRecovery = false;
};
