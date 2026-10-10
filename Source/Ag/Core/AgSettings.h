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
 * Points at levels and shared data assets so code never hardcodes asset paths.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Ag"))
class UAgSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UAgSettings* Get() { return GetDefault<UAgSettings>(); }

	/** DA_Camera, loaded on first use and kept loaded from then on. */
	const UAgCameraData* GetCameraData() const;

	/** DA_CombatRules, loaded on first use and kept loaded from then on. */
	const UAgCombatRules* GetCombatRules() const;

	/** TP_Boss, loaded on first use and kept loaded from then on. */
	const UTargetingPreset* GetBossTargeting() const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/** 타이틀 화면 레벨 (게임 플로우 'Front End') */
	UPROPERTY(Config, EditAnywhere, Category = "Maps")
	TSoftObjectPtr<UWorld> FrontEndMap;

	/** 보스전 레벨 (게임 플로우 'Boss Stage') */
	UPROPERTY(Config, EditAnywhere, Category = "Maps")
	TSoftObjectPtr<UWorld> BossStageMap;

	/** DA_Camera */
	UPROPERTY(Config, EditAnywhere, Category = "Data")
	TSoftObjectPtr<UAgCameraData> CameraData;

	/** DA_CombatRules */
	UPROPERTY(Config, EditAnywhere, Category = "Data")
	TSoftObjectPtr<UAgCombatRules> CombatRules;

	/** TP_Boss: finds boss candidates around the player for 공격 중 이동 and lock-on. Each use then checks its own range. */
	UPROPERTY(Config, EditAnywhere, Category = "Data")
	TSoftObjectPtr<UTargetingPreset> BossTargeting;

private:
	/**
	 * The data assets above once loaded. Only soft pointers lead to them, so outside the editor every garbage collection
	 * would unload them and the next use would load them again on the game thread.
	 */
	UPROPERTY(Transient)
	mutable TObjectPtr<UAgCameraData> LoadedCameraData;

	UPROPERTY(Transient)
	mutable TObjectPtr<UAgCombatRules> LoadedCombatRules;

	UPROPERTY(Transient)
	mutable TObjectPtr<UTargetingPreset> LoadedBossTargeting;
};
