// Copyright Woogle. All Rights Reserved.

#include "Combat/AgTimeSubsystem.h"

#include "Ag.h"
#include "Combat/AgCombatLibrary.h"
#include "Data/AgCombatRules.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
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

	if (bSlowMotionPending)
	{
		bSlowMotionPending = false;
		ApplySlowMotion(PendingSpeed, PendingDuration);
	}
}

void UAgTimeSubsystem::StartSlowMotion(float Speed, float RealDuration)
{
	if (RealDuration <= 0.f)
	{
		return;
	}
	if (GetWorld()->GetTimerManager().IsTimerActive(HitstopTimer))
	{
		bSlowMotionPending = true;
		PendingSpeed = Speed;
		PendingDuration = RealDuration;
		return;
	}
	ApplySlowMotion(Speed, RealDuration);
}

void UAgTimeSubsystem::ApplySlowMotion(float Speed, float RealDuration)
{
	// Global time dilation slows everything that runs on game time: motion, movement, timers, camera shakes, the HUD's
	// screen effects. The end is counted in real time.
	GetWorld()->GetWorldSettings()->SetTimeDilation(Speed);
	SlowMotionEndTime = FPlatformTime::Seconds() + RealDuration;
	UE_LOG(LogAg, Verbose, TEXT("Slow motion starts: speed %.2f for %.2f s of real time"), Speed, RealDuration);
}

void UAgTimeSubsystem::EndSlowMotion()
{
	SlowMotionEndTime = -1.0;
	GetWorld()->GetWorldSettings()->SetTimeDilation(1.f);
	UE_LOG(LogAg, Verbose, TEXT("Slow motion ends"));
}

void UAgTimeSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (SlowMotionEndTime >= 0.0 && FPlatformTime::Seconds() >= SlowMotionEndTime)
	{
		EndSlowMotion();
	}
}

TStatId UAgTimeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAgTimeSubsystem, STATGROUP_Tickables);
}
