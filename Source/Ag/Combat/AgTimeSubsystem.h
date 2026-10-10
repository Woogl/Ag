// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AgTimeSubsystem.generated.h"

/** 전투 시스템 '히트스톱' and 플레이어 사양 '슬로우모션'. */
UCLASS()
class UAgTimeSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * Stops the given characters' motion and movement for the hitstop time, counted in game time.
	 * Calling it again while it runs restarts the count.
	 */
	void StartHitstop(std::initializer_list<AActor*> Actors);

	/**
	 * Runs the whole game at Speed for RealDuration seconds of real time. Starting again while it runs restarts the
	 * count. Started during a hitstop, it begins when the hitstop ends (패리).
	 */
	void StartSlowMotion(float Speed, float RealDuration);

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

private:
	void EndHitstop();
	void ApplySlowMotion(float Speed, float RealDuration);
	void EndSlowMotion();

	TArray<TWeakObjectPtr<AActor>> FrozenActors;
	FTimerHandle HitstopTimer;

	/** Real time the slow motion ends, or negative when none runs. */
	double SlowMotionEndTime = -1.0;

	/** A slow motion waiting for the hitstop to end. */
	bool bSlowMotionPending = false;
	float PendingSpeed = 1.f;
	float PendingDuration = 0.f;
};
