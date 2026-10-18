// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AgTargetingPresetData.generated.h"

class UTargetingPreset;

/** TargetingPresets.json의 값: 프리셋 ID마다 읽을 때 만든 TargetingPreset(구현 계획서 'JSON 데이터'). 행동의 Targeting이 ID로 가리킵니다. */
UCLASS()
class UAgTargetingPresetSet : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, VisibleAnywhere, Category = "Targeting")
	TMap<FName, TObjectPtr<UTargetingPreset>> Presets;
};
