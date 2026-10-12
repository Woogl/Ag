// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AgCombatRules.generated.h"

/**
 * CombatRules.json: 모든 캐릭터에 공통인 전투 시스템 규칙(구현 계획서 'JSON과 DataAsset').
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

	/** 가드 경감 배율 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Hit", meta = (ClampMin = 0, ClampMax = 1))
	float GuardReduction = 1.f;

	/** 방어 상수: HP·PP 감소량 계산식의 DEF에 따른 감소 비율 DefenseConstant / (DefenseConstant + DEF) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Hit", meta = (ClampMin = 1))
	float DefenseConstant = 0.f;

	/** 넉업: 땅에서 맞았을 때의 체공 시간. 중력과 관계없이 정한 포물선을 따릅니다. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Hit", meta = (Units = "s", ClampMin = 0))
	float KnockUpAirTime = 0.f;

	/** 넉업: 최고 높이 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Hit", meta = (Units = "cm", ClampMin = 0))
	float KnockUpHeight = 0.f;

	/** PP 리젠: 마지막으로 PP가 줄어든 뒤의 리젠 대기 시간. 모든 캐릭터가 같습니다. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Regen", meta = (Units = "s", ClampMin = 0))
	float PPRegenDelay = 0.f;

	/** 그로기 지속 시간. 그로기 모션의 시작부터 원래 자세로 돌아오는 모션의 끝까지 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Groggy", meta = (Units = "s", ClampMin = 0))
	float GroggyDuration = 0.f;
};
