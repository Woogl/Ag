// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AgTimeSubsystem.generated.h"

class AAgCharacterBase;

DECLARE_MULTICAST_DELEGATE_OneParam(FAgTimeStopChanged, bool);

/**
 * 시간 정지는 엔진·GAS 타이머(그로기 시간, 쿨다운, 리젠)도 멈추도록 캐릭터마다가 아니라 게임 전체 시간 배율로 겁니다.
 */
UCLASS()
class UAgTimeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void StartSlowMotion(float Speed, float RealDuration);

	bool StartTimeStop(UObject& Requester, AAgCharacterBase& Exempt);

	void EndTimeStop(UObject& Requester);

	bool IsTimeStopped() const
	{
		return bTimeStopped;
	}

	float GetTimeStopCompensation() const;

	FAgTimeStopChanged OnTimeStopChanged;

private:
	/** 시간 정지 중에 부르면 시간 정지 배율을 덮으므로, 시간 정지가 아닐 때만 부릅니다. */
	void ApplySlowMotion(float Speed, float RealDuration);

	void EndSlowMotion();

	FTimerHandle SlowMotionTimer;

	float PendingSlowMotionSpeed = 1.f;
	float PendingSlowMotionDuration = 0.f;

	bool bTimeStopped = false;
	TWeakObjectPtr<UObject> TimeStopRequester;

	TWeakObjectPtr<AAgCharacterBase> TimeStopExempt;
};
