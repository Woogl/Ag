// Copyright Woogle. All Rights Reserved.

#include "Core/AgSettings.h"

#include "Data/AgCameraData.h"
#include "Data/AgCombatRules.h"
#include "Data/AgDataFile.h"
#include "TargetingSystem/TargetingPreset.h"
#include "UObject/Package.h"

namespace
{
	/** Reads a common data file into the one object of its class (구현 계획서 '데이터'). */
	template <typename TData>
	TData* LoadCommonData(const TCHAR* ObjectName, const FString& FileName)
	{
		// A fixed name in the transient package keeps one object per session and makes it easy to find for inspection.
		TData* Data = FindObject<TData>(GetTransientPackage(), ObjectName);
		if (!Data)
		{
			Data = NewObject<TData>(GetTransientPackage(), ObjectName);
		}
		AgDataFile::Load(*Data, FileName);
		return Data;
	}
}

const UAgCameraData* UAgSettings::GetCameraData() const
{
	if (!LoadedCameraData)
	{
		LoadedCameraData = LoadCommonData<UAgCameraData>(TEXT("AgCameraData"), CameraDataFile);
	}
	return LoadedCameraData;
}

const UAgCombatRules* UAgSettings::GetCombatRules() const
{
	if (!LoadedCombatRules)
	{
		LoadedCombatRules = LoadCommonData<UAgCombatRules>(TEXT("AgCombatRules"), CombatRulesFile);
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

	// A changed data file is read into the same object; a changed asset is loaded on its next use.
	const FName Property = PropertyChangedEvent.GetMemberPropertyName();
	if (Property == GET_MEMBER_NAME_CHECKED(UAgSettings, CameraDataFile) && LoadedCameraData)
	{
		AgDataFile::Load(*LoadedCameraData, CameraDataFile);
	}
	if (Property == GET_MEMBER_NAME_CHECKED(UAgSettings, CombatRulesFile) && LoadedCombatRules)
	{
		AgDataFile::Load(*LoadedCombatRules, CombatRulesFile);
	}
	LoadedBossTargeting = nullptr;
}
#endif
