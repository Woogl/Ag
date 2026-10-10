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

	/**
	 * Speed the ragdoll is pushed with along the last attack's direction (전투 시스템 '사망 처리' 6).
	 * The spec gives no number; this is an implementation value for QA to tune.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Death", meta = (Units = "cm/s", ClampMin = 0))
	float DeathPushSpeed = 0.f;
};
