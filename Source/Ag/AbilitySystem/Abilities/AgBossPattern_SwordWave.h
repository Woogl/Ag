// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AgBossPatternAbility.h"
#include "AgBossPattern_SwordWave.generated.h"

/**
 * A6 검기 발사: 세 번 휘두르며, 휘두를 때마다 발사 노티파이에서 그 타의 공격 데이터로 검기 하나를 정면으로 쏩니다.
 * 보스는 판정 구간 전처럼 발사마다 그 전에 회전을 멈추므로, 검기는 그때 바라본 쪽으로 날아갑니다.
 */
UCLASS()
class UAgBossPattern_SwordWave : public UAgBossPatternAbility
{
	GENERATED_BODY()

protected:
	virtual void OnPatternStarted() override;
	virtual void GetHitStarts(TArray<FAgHitStart>& OutHitStarts) const override;

private:
	UFUNCTION()
	void HandleRelease(FGameplayEventData Payload);

	int32 WaveIndex = 0;
};
