// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AgCombatRules.generated.h"

/** DA_CombatRules: rules of 전투 시스템 shared by every character. */
UCLASS(BlueprintType)
class UAgCombatRules : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 히트스톱 시간 (게임 시간) */
	UPROPERTY(EditDefaultsOnly, Category = "Hit", meta = (Units = "s", ClampMin = 0))
	float HitstopTime = 0.f;

	/** 가드 경감 배율 (GuardReduction) */
	UPROPERTY(EditDefaultsOnly, Category = "Hit", meta = (ClampMin = 0, ClampMax = 1))
	float GuardReduction = 1.f;

	/** PP 리젠: 리젠 대기 시간 after the last PP decrease. The same for every character. */
	UPROPERTY(EditDefaultsOnly, Category = "Regen", meta = (Units = "s", ClampMin = 0))
	float PPRegenDelay = 0.f;

	/**
	 * Regen is applied in steps this long (rate per second × interval each step).
	 * The spec gives no number; an implementation value, short enough that the bars fill smoothly.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Regen", meta = (Units = "s", ClampMin = 0.01))
	float RegenTickInterval = 0.1f;

	/** 그로기 지속 시간, from the groggy motion's start to the end of the motion back to the stance. */
	UPROPERTY(EditDefaultsOnly, Category = "Groggy", meta = (Units = "s", ClampMin = 0))
	float GroggyDuration = 0.f;

	/**
	 * Speed the ragdoll is pushed with along the last attack's direction (전투 시스템 '사망 처리' 6).
	 * The spec gives no number; this is an implementation value for QA to tune.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Death", meta = (Units = "cm/s", ClampMin = 0))
	float DeathPushSpeed = 0.f;
};
