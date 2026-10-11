// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AgSettings.generated.h"

class UTargetingPreset;

/**
 * Ag 프로젝트 설정(Project Settings > Game > Ag).
 * 레벨, 공통 데이터 파일, 공용 에셋을 가리켜 코드가 경로를 하드코딩하지 않게 합니다.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Ag"))
class UAgSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UAgSettings* Get() { return GetDefault<UAgSettings>(); }

	/** 타이틀 화면 레벨 (게임 플로우 'Front End') */
	UPROPERTY(Config, EditAnywhere, Category = "Maps")
	TSoftObjectPtr<UWorld> FrontEndMap;

	/** 보스전 레벨 (게임 플로우 'Boss Stage') */
	UPROPERTY(Config, EditAnywhere, Category = "Maps")
	TSoftObjectPtr<UWorld> BossStageMap;

	/** Camera.json: 프로젝트 Data 폴더의 파일. 에셋 매니저가 엔진 초기화 때 읽습니다. */
	UPROPERTY(Config, EditAnywhere, Category = "Data")
	FString CameraDataFile;

	/** CombatRules.json: 프로젝트 Data 폴더의 파일. 에셋 매니저가 엔진 초기화 때 읽습니다. */
	UPROPERTY(Config, EditAnywhere, Category = "Data")
	FString CombatRulesFile;

	/** TP_Enemy: 공격 보정, 락온, 처형의 대상 후보(플레이어 주변의 캐릭터)를 모읍니다. 범위와 조건은 쓰는 곳마다 따로 확인합니다. */
	UPROPERTY(Config, EditAnywhere, Category = "Data")
	TSoftObjectPtr<UTargetingPreset> EnemyTargeting;

	/**
	 * Regen is applied in steps this long (rate per second × interval each step).
	 * The spec gives no number; an implementation value, short enough that the bars fill smoothly.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Combat", meta = (Units = "s", ClampMin = 0.01))
	float RegenTickInterval = 0.1f;

	/**
	 * Speed the ragdoll is pushed with along the last attack's direction (전투 시스템 '사망 처리' 6).
	 * The spec gives no number; an implementation value.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Combat", meta = (Units = "cm/s", ClampMin = 0))
	float DeathPushSpeed = 0.f;
};
