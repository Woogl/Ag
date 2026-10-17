// Copyright Woogle. All Rights Reserved.

#pragma once

#include "Camera/CameraShakeBase.h"
#include "AgCameraShake.generated.h"

struct FAgCameraShakeSettings;
class UAgCameraData;

UCLASS(meta = (ScriptName = "AgCameraShakeEffect"))
class UAgCameraShake : public UCameraShakeBase
{
	GENERATED_BODY()

public:
	UAgCameraShake(const FObjectInitializer& ObjectInitializer);
	void Initialize(const FAgCameraShakeSettings& Settings, const UAgCameraData& Camera);
};
