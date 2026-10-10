// Copyright Woogle. All Rights Reserved.

#include "Core/AgSettings.h"

#include "Data/AgCameraData.h"
#include "Data/AgCombatRules.h"
#include "TargetingSystem/TargetingPreset.h"

const UAgCameraData* UAgSettings::GetCameraData() const
{
	if (!LoadedCameraData)
	{
		LoadedCameraData = CameraData.LoadSynchronous();
	}
	return LoadedCameraData;
}

const UAgCombatRules* UAgSettings::GetCombatRules() const
{
	if (!LoadedCombatRules)
	{
		LoadedCombatRules = CombatRules.LoadSynchronous();
	}
	return LoadedCombatRules;
}

const UTargetingPreset* UAgSettings::GetBossTargeting() const
{
	if (!LoadedBossTargeting)
	{
		LoadedBossTargeting = BossTargeting.LoadSynchronous();
	}
	return LoadedBossTargeting;
}

#if WITH_EDITOR
void UAgSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// A changed asset is loaded on its next use.
	LoadedCameraData = nullptr;
	LoadedCombatRules = nullptr;
	LoadedBossTargeting = nullptr;
}
#endif
