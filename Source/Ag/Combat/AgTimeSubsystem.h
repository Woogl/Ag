// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AgTimeSubsystem.generated.h"

class AAgCharacterBase;

/**
 * 시간 정지는 엔진·GAS 타이머(그로기 시간, 쿨다운, 리젠)도 멈추도록 캐릭터마다가 아니라 게임 전체 시간 배율로 겁니다.
 */
UCLASS()
class UAgTimeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void StartSlowMotion(float Speed, float RealDuration);

	/** 요청한 캐릭터는 시간 정지 중에도 움직입니다. */
	void StartTimeStop(AAgCharacterBase& Requester);

	void EndTimeStop(AAgCharacterBase& Requester);

	float GetSlowMotionDuration() const
	{
		return SlowMotionDuration;
	}

private:
	float GetTimeStopCompensation() const;

	/** 시간 정지 중에 부르면 시간 정지 배율을 덮으므로, 시간 정지가 아닐 때만 부릅니다. */
	void ApplySlowMotion(float Speed, float RealDuration);

	void EndSlowMotion();

	FTimerHandle SlowMotionTimer;

	float SlowMotionDuration = 0.f;

	float PendingSlowMotionSpeed = 1.f;
	float PendingSlowMotionDuration = 0.f;

	bool bTimeStopped = false;
	TWeakObjectPtr<AAgCharacterBase> TimeStopRequester;
};
