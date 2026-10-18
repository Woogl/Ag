// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AgCombatRules.generated.h"

/**
 * CombatRules.json: 모든 캐릭터에 공통인 전투 시스템 규칙(구현 계획서 'JSON 데이터').
 * 모든 필드는 Transient이고 에셋 매니저가 JSON에서 채웁니다. 객체는 에셋 매니저가 들고 있고, UAgAssetManager::GetCombatRules로 읽습니다.
 */
UCLASS(BlueprintType)
class UAgCombatRules : public UObject
{
	GENERATED_BODY()

public:
	/** 가드 경감 배율 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Hit", meta = (ClampMin = 0, ClampMax = 1))
	float GuardReduction = 1.f;

	/** 방어 상수: HP·PP 감소량 계산식의 DEF에 따른 감소 비율 DefenseConstant / (DefenseConstant + DEF) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Hit", meta = (ClampMin = 1))
	float DefenseConstant = 0.f;

	/** HUD 감소 잔상: 바가 줄어든 뒤 잔상이 기다리는 시간 (게임 시간) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "HUD|Trail", meta = (Units = "s", ClampMin = 0))
	float HUDTrailDelay = 0.f;

	/** HUD 감소 잔상: 기다린 뒤 바의 현재 값까지 줄어드는 시간 (게임 시간) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "HUD|Trail", meta = (Units = "s", ClampMin = 0))
	float HUDTrailShrinkTime = 0.f;
};
