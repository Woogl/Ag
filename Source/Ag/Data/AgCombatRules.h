// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AgCombatRules.generated.h"

/**
 * CombatRules.json: rules of 전투 시스템 shared by every character (구현 계획서 '데이터').
 * Every field is Transient and comes from the data file (AgDataFile); UAgSettings holds the one object.
 */
UCLASS(BlueprintType)
class UAgCombatRules : public UObject
{
	GENERATED_BODY()

public:
	/** 히트스톱 시간 (게임 시간) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Hit", meta = (Units = "s", ClampMin = 0))
	float HitstopTime = 0.f;

	/** 가드 경감 배율 (GuardReduction) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Hit", meta = (ClampMin = 0, ClampMax = 1))
	float GuardReduction = 1.f;

	/** 넉업: 체공 시간 when hit on the ground. The flight follows its own parabola, whatever the gravity. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Hit", meta = (Units = "s", ClampMin = 0))
	float KnockUpAirTime = 0.f;

	/** 넉업: 최고 높이 of the flight. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Hit", meta = (Units = "cm", ClampMin = 0))
	float KnockUpHeight = 0.f;

	/** PP 리젠: 리젠 대기 시간 after the last PP decrease. The same for every character. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Regen", meta = (Units = "s", ClampMin = 0))
	float PPRegenDelay = 0.f;

	/** 그로기 지속 시간, from the groggy motion's start to the end of the motion back to the stance. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Groggy", meta = (Units = "s", ClampMin = 0))
	float GroggyDuration = 0.f;
};
