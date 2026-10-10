// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AgTimeSubsystem.generated.h"

/** 전투 시스템 '히트스톱' (and the player's 슬로우모션 later). */
UCLASS()
class UAgTimeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * Stops the given characters' motion and movement for the hitstop time, counted in game time.
	 * Calling it again while it runs restarts the count.
	 */
	void StartHitstop(std::initializer_list<AActor*> Actors);

private:
	void EndHitstop();

	TArray<TWeakObjectPtr<AActor>> FrozenActors;
	FTimerHandle HitstopTimer;
};
