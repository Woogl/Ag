// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AgSettings.generated.h"

class UAgCameraData;
class UAgCombatRules;
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

	/**
	 * Camera.json. 처음 쓸 때 UAgCameraData의 기본 객체에 읽어 둡니다(구현 계획서 '데이터'). 읽지 못하면 오류를 알리고 기본값을 줍니다.
	 * 객체를 따로 만들어 가리키지 않는 것은, 패키징 빌드에서 이 설정의 기본 객체가 GC가 참조를 따라가지 않는 객체 풀에 있기 때문입니다.
	 */
	const UAgCameraData& GetCameraData() const;

	/** CombatRules.json. GetCameraData와 같습니다. */
	const UAgCombatRules& GetCombatRules() const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/** 타이틀 화면 레벨 (게임 플로우 'Front End') */
	UPROPERTY(Config, EditAnywhere, Category = "Maps")
	TSoftObjectPtr<UWorld> FrontEndMap;

	/** 보스전 레벨 (게임 플로우 'Boss Stage') */
	UPROPERTY(Config, EditAnywhere, Category = "Maps")
	TSoftObjectPtr<UWorld> BossStageMap;

	/** Camera.json: a file in the project's Data folder. */
	UPROPERTY(Config, EditAnywhere, Category = "Data")
	FString CameraDataFile;

	/** CombatRules.json: a file in the project's Data folder. */
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

private:
	/** 위 파일을 데이터 클래스의 기본 객체에 읽었는지. */
	mutable bool bCameraDataLoaded = false;
	mutable bool bCombatRulesLoaded = false;
};
