// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AgBossPatternAbility.h"
#include "AgBossPattern_SwordWave.generated.h"

/**
 * A6 검기 발사: three swings, each firing one sword wave straight ahead at its release notify, with that hit's attack data.
 * The boss stops turning before each release like before an attack window, so the wave flies where it faced then.
 */
UCLASS()
class UAgBossPattern_SwordWave : public UAgBossPatternAbility
{
	GENERATED_BODY()

protected:
	virtual void OnPatternStarted() override;
	virtual void GetParryableHitStarts(TArray<float>& OutTimes) const override;

private:
	UFUNCTION()
	void HandleRelease(FGameplayEventData Payload);

	int32 WaveIndex = 0;
};
