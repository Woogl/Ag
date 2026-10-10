// Copyright Woogle. All Rights Reserved.

#include "Combat/AgTimeSubsystem.h"

#include "Combat/AgCombatLibrary.h"
#include "Data/AgCombatRules.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace
{
	/** Per-actor time scale during hitstop: motion, montages and movement all but stop. */
	constexpr float HitstopTimeDilation = 1.e-4f;
}

void UAgTimeSubsystem::StartHitstop(std::initializer_list<AActor*> Actors)
{
	const UAgCombatRules* Rules = UAgCombatLibrary::GetCombatRules();
	if (!Rules || Rules->HitstopTime <= 0.f)
	{
		return;
	}

	for (AActor* Actor : Actors)
	{
		if (Actor)
		{
			Actor->CustomTimeDilation = HitstopTimeDilation;
			FrozenActors.AddUnique(Actor);
		}
	}
	GetWorld()->GetTimerManager().SetTimer(HitstopTimer, this, &ThisClass::EndHitstop, Rules->HitstopTime, false);
}

void UAgTimeSubsystem::EndHitstop()
{
	for (const TWeakObjectPtr<AActor>& Actor : FrozenActors)
	{
		if (Actor.IsValid())
		{
			Actor->CustomTimeDilation = 1.f;
		}
	}
	FrozenActors.Reset();
}
