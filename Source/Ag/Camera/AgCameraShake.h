// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AgCameraShake.generated.h"

class AActor;
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
	/** 셰이크 큐는 월드 재생이라, 엔진이 Source에서 각 플레이어까지의 거리로 세기를 줄여 재생합니다. */
	void Play(AActor* Source, EAgCameraShake Shake);
}
