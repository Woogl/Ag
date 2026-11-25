// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/AgJsonData.h"
#include "Types/TargetingSystemTypes.h"
#include "AgTargetingPresetData.generated.h"

class UTargetingPreset;

UCLASS()
class UAgTargetingPresetSet : public UAgJsonData
{
	GENERATED_BODY()

public:
	static const UTargetingPreset* FindPreset(FName Id);

	virtual const TCHAR* GetFileName() const override
	{
		return TEXT("TargetingPresets.json");
	}

	virtual bool IsAlwaysLoaded() const override
	{
		return true;
	}

	virtual bool ReadJson(FAgJsonReader& Reader, FName Key) override;

	virtual void CopyJson(const UAgJsonData& Loaded) override;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Targeting")
	TMap<FName, FTargetingTaskSet> Definitions;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Targeting")
	TMap<FName, TObjectPtr<UTargetingPreset>> Presets;
};
