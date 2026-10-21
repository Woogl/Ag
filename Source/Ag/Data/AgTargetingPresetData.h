// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/TargetingSystemTypes.h"
#include "AgTargetingPresetData.generated.h"

class UTargetingPreset;

UCLASS()
class UAgTargetingPresetSet : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, VisibleAnywhere, Category = "Targeting")
	TMap<FName, FTargetingTaskSet> Definitions;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Targeting")
	TMap<FName, TObjectPtr<UTargetingPreset>> Presets;
};
