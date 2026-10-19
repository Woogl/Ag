// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AgTargetingPresetData.generated.h"

class UTargetingPreset;
class UTargetingTask;

USTRUCT()
struct FAgTargetingPresetDefinition
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<UTargetingTask>> Tasks;
};

UCLASS()
class UAgTargetingPresetSet : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, VisibleAnywhere, Category = "Targeting")
	TMap<FName, FAgTargetingPresetDefinition> Definitions;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Targeting")
	TMap<FName, TObjectPtr<UTargetingPreset>> Presets;
};
