// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AgTimeSubsystem.generated.h"

/** 게임 전체의 시간 배율을 맡습니다(플레이어 사양 '슬로우모션'). 캐릭터마다 멈추는 히트스톱은 캐릭터의 태그가 맡습니다. */
UCLASS()
class UAgTimeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** 실제 시간으로 RealDuration초 동안 게임 전체를 Speed배로 돌립니다. 도는 중에 다시 시작하면 처음부터 다시 셉니다. */
	void StartSlowMotion(float Speed, float RealDuration);

private:
	void EndSlowMotion();

	FTimerHandle SlowMotionTimer;
};
