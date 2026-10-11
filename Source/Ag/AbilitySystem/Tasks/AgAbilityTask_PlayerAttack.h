// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgAttackData.h"
#include "AgAbilityTask_PlayerAttack.generated.h"

class AAgCharacterBase;
class UAbilityTask_PlayMontageAndWait;
class UAgAbilityTask_AttackMovement;
class UAnimMontage;

/**
 * 플레이어 공격 하나(평타 한 타, 회피 반격, 스킬, 궁극기): 공격 데이터를 판정 구간에 걸고 몽타주를 이어 재생하며, 공격 중
 * 이동 태스크를 돌리고 범위 안에 적이 없는 타의 공격 방향을 정합니다(플레이어 사양 '공격 방향', '공격 중 이동').
 * 마지막 판정 구간이 끝나면 OnLastWindowEnded, 마지막 모션이 끝나면 OnCompleted, 모션이 끊기면 OnInterrupted를 보냅니다.
 * 행동 중과 후딜, 어빌리티를 끝낼지는 어빌리티가 정합니다. 끝나면(어빌리티가 끝날 때 포함) 공격 데이터를 지웁니다.
 */
UCLASS()
class UAgAbilityTask_PlayerAttack : public UAbilityTask
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FGenericGameplayTaskDelegate OnLastWindowEnded;

	UPROPERTY(BlueprintAssignable)
	FGenericGameplayTaskDelegate OnCompleted;

	UPROPERTY(BlueprintAssignable)
	FGenericGameplayTaskDelegate OnInterrupted;

	/** Attack의 타로 InMontages를 차례로 재생합니다. 다음 몽타주는 앞 몽타주가 블렌드 아웃을 시작할 때 이어 붙습니다. */
	static UAgAbilityTask_PlayerAttack* PlayAttack(UGameplayAbility* OwningAbility, const FAgPlayerAttack& Attack, TConstArrayView<TObjectPtr<UAnimMontage>> InMontages);

	/** 지금 타의 대상: 타를 시작할 때 가장 가까운 적(정지 간격의 대상)입니다. 없으면 null입니다. */
	AAgCharacterBase* GetHitTarget() const;

	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	void PlayMontageAt(int32 Index);

	/** 공격 방향: 타겟팅 범위 안에 적이 없으면 락온 중일 때만 락온 대상 쪽으로 돌고, 아니면 정면으로 갑니다. */
	UFUNCTION()
	void HandleHitStarted();

	void HandleAttackWindowEnded(const FGameplayEventData* Payload);

	/** 다음 몽타주는 앞 몽타주가 블렌드 아웃을 시작할 때 시작합니다(이어 재생하는 몽타주는 끝에서 블렌드 아웃합니다). */
	UFUNCTION()
	void HandleMontageBlendingOut();

	/** 모션이 끝났습니다. 후딜은 블렌드 아웃을 지나 여기까지입니다. */
	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageCancelled();

	TArray<FAgPlayerAttackHit> Hits;
	float TargetingRange = 0.f;
	float MaxApproachDistance = 0.f;

	UPROPERTY()
	TArray<TObjectPtr<UAnimMontage>> Montages;

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	UPROPERTY()
	TObjectPtr<UAgAbilityTask_AttackMovement> MovementTask;

	int32 MontageIndex = 0;
	FDelegateHandle WindowEndedHandle;
};
