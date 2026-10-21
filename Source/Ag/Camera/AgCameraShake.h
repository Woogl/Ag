// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AgCameraShake.generated.h"

class APawn;
UENUM()
enum class EAgCameraShake : uint8
{
	None UMETA(DisplayName = "없음"),
	Small UMETA(DisplayName = "작음"),
	Medium UMETA(DisplayName = "보통"),
	Large UMETA(DisplayName = "큼"),
};

namespace AgCameraShake
{
	void Play(APawn* Target, EAgCameraShake Shake);
}
