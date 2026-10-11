// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AgCombatRules.generated.h"

/**
 * CombatRules.json: 모든 캐릭터에 공통인 전투 시스템 규칙(구현 계획서 '데이터').
 * 모든 필드는 Transient이고 에셋 매니저가 JSON에서 채웁니다. 객체는 에셋 매니저가 들고 있고, UAgAssetManager::GetCombatRules로 읽습니다.
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
