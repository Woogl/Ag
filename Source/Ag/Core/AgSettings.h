// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AgSettings.generated.h"

class UAgCameraData;
class UAgCombatRules;
class UTargetingPreset;

/**
 * Project settings for Ag (Project Settings > Game > Ag).
 * Points at levels, the common data files and shared assets so code never hardcodes their paths.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Ag"))
class UAgSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UAgSettings* Get() { return GetDefault<UAgSettings>(); }

	/** Camera.json, read on first use into one object kept from then on (구현 계획서 '데이터'). */
	const UAgCameraData* GetCameraData() const;

	/** CombatRules.json, read on first use into one object kept from then on. */
	const UAgCombatRules* GetCombatRules() const;

	/** TP_Enemy. 처음 쓸 때 로드해 계속 들고 있습니다. */
	const UTargetingPreset* GetEnemyTargeting() const;

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
	/** The common data read from the files above. A file read again fills the same object, so users keep their pointers. */
	UPROPERTY(Transient)
	mutable TObjectPtr<UAgCameraData> LoadedCameraData;

	UPROPERTY(Transient)
	mutable TObjectPtr<UAgCombatRules> LoadedCombatRules;

	/** 로드한 TP_Enemy. 소프트 포인터만 가리키므로, 에디터 밖에서는 가비지 컬렉션마다 언로드됩니다. */
	UPROPERTY(Transient)
	mutable TObjectPtr<UTargetingPreset> LoadedEnemyTargeting;
};
