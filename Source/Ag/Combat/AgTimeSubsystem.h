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
	 * 히트스톱 시간 동안 주어진 캐릭터들의 모션과 이동을 멈춥니다. 시간은 게임 시간으로 캐릭터마다 따로 세며, 이미 멈춘
	 * 캐릭터가 다시 걸리면 그 캐릭터만 처음부터 다시 셉니다.
	 */
	void StartHitstop(std::initializer_list<AActor*> Actors);

	/** Runs the whole game at Speed for RealDuration seconds of real time. Starting again while it runs restarts the count. */
	void StartSlowMotion(float Speed, float RealDuration);

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

private:
	/** 그 캐릭터의 히트스톱을 끝냅니다. */
	void EndHitstop(TWeakObjectPtr<AActor> Actor);
	void EndSlowMotion();

	/** 히트스톱 중인 캐릭터마다 끝나는 시점을 세는 타이머 */
	TMap<TWeakObjectPtr<AActor>, FTimerHandle> HitstopTimers;

	/** Real time the slow motion ends, or negative when none runs. */
	double SlowMotionEndTime = -1.0;
};
