// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgBossPattern_SwordWave.generated.h"

class UAgAbilityTask_HoldTag;

/**
 * 검기 발사: 세 번 휘두르며, 휘두를 때마다 발사 노티파이에서 그 타의 공격 데이터로 검기 하나를 정면으로 쏩니다.
 * 발사를 판정 구간처럼 보아 발사마다 회전 정지 시간만큼 앞서 회전을 멈추므로, 검기는 그때 바라본 쪽으로 날아갑니다.
 * 정지 간격은 공용 패턴과 같이 지킵니다.
 */
UCLASS()
class UAgBossPattern_SwordWave : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgBossPattern_SwordWave();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	/** 다음 발사보다 회전 정지 시간만큼 앞선 몽타주 위치를 기다립니다. */
	void WaitRotationStop();

	UFUNCTION()
	void HandleRotationStop();

	UFUNCTION()
	void HandleRelease(FGameplayEventData Payload);

	UFUNCTION()
	void HandleMontageFinished();

	UFUNCTION()
	void HandleMontageCancelled();

	/** 발사 노티파이의 몽타주 시각 (빠른 순) */
	TArray<float> ReleaseTimes;

	/** 발사 전 회전 정지 (State.Boss.RotationLocked) */
	UPROPERTY()
	TObjectPtr<UAgAbilityTask_HoldTag> RotationLockTag;

	int32 WaveIndex = 0;
};
