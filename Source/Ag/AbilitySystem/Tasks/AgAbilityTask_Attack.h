// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgAttackData.h"
#include "AgAbilityTask_Attack.generated.h"

class AAgCharacterBase;
class UAbilityTask_PlayMontageAndWait;
class UAgAbilityTask_AttackMovement;
class UAnimMontage;

/**
 * 판정 구간이 있는 공격 하나(평타 한 타, 회피 반격, 스킬, 보스 공용 패턴, 구현 계획서 '공격 진행'): 공격 데이터를 판정 구간에
 * 걸고 몽타주를 재생하며, 공격 중 이동 태스크를 돌립니다. 마지막 판정 구간이 끝나면 OnLastWindowEnded, 모션이 끝나면
 * OnCompleted, 모션이 끊기면 OnInterrupted를 보냅니다. 행동 중과 후딜, 어빌리티를 끝낼지는 어빌리티가 정합니다.
 * 끝나면(어빌리티가 끝날 때 포함) 공격 데이터를 지웁니다.
 */
UCLASS()
class UAgAbilityTask_Attack : public UAbilityTask
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FGenericGameplayTaskDelegate OnLastWindowEnded;

	UPROPERTY(BlueprintAssignable)
	FGenericGameplayTaskDelegate OnCompleted;

	UPROPERTY(BlueprintAssignable)
	FGenericGameplayTaskDelegate OnInterrupted;

	/** 플레이어 공격 Attack의 타로 InMontage를 재생합니다. 행동마다 몽타주는 하나입니다(구현 계획서 '몽타주'). */
	static UAgAbilityTask_Attack* PlayAttack(UGameplayAbility* OwningAbility, const FAgPlayerAttack& Attack, UAnimMontage* InMontage);

	/** 보스 패턴 Pattern의 타로 InMontage를 InPlayRate로 재생합니다. 보스 패턴에는 타겟팅 범위가 없습니다. */
	static UAgAbilityTask_Attack* PlayAttack(UGameplayAbility* OwningAbility, const FAgBossPattern& Pattern, UAnimMontage* InMontage, float InPlayRate);

	/** 지금 타의 대상: 타를 시작할 때 정한 공격 중 이동의 대상(정지 간격의 대상)입니다. 없으면 null입니다. */
	AAgCharacterBase* GetHitTarget() const;

	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	void HandleAttackWindowEnded(const FGameplayEventData* Payload);

	/** 모션이 끝났습니다. 후딜은 블렌드 아웃을 지나 여기까지입니다. */
	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageCancelled();

	/** 판정 구간마다 하나씩. 보스 공격의 플레이어 전용 값은 기본값입니다. */
	TArray<FAgPlayerAttackHit> Hits;

	/** 음수면 범위가 없습니다(보스). */
	float TargetingRange = -1.f;

	float MaxApproachDistance = 0.f;
	float PlayRate = 1.f;

	UPROPERTY()
	TObjectPtr<UAnimMontage> Montage;

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	UPROPERTY()
	TObjectPtr<UAgAbilityTask_AttackMovement> MovementTask;

	FDelegateHandle WindowEndedHandle;
};
