// Copyright Woogle. All Rights Reserved.

#include "Data/AgTargetingPresetData.h"

#include "TargetingSystem/TargetingPreset.h"

const UTargetingPreset* UAgTargetingPresetSet::FindPreset(FName Id)
{
	return UAgAssetManager::Get().GetData<UAgTargetingPresetSet>().Presets.FindRef(Id);
}

bool UAgTargetingPresetSet::ReadJson(FAgJsonReader& Reader, FName Key)
{
	return ReadJsonInto(Reader, GET_MEMBER_NAME_CHECKED(UAgTargetingPresetSet, Definitions));
}

void UAgTargetingPresetSet::CopyJson(const UAgJsonData& Loaded)
{
	Super::CopyJson(Loaded);

	Presets.Reset();
	for (const TPair<FName, FTargetingTaskSet>& Pair : Definitions)
	{
		UTargetingPreset* Preset = NewObject<UTargetingPreset>(this);
		Preset->GetMutableTargetingTaskSet() = Pair.Value;
		Presets.Add(Pair.Key, Preset);
	}
}
