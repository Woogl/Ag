// Copyright Woogle. All Rights Reserved.

#include "Combat/AgTimeSubsystem.h"

#include "Ag.h"
#include "Core/AgSettings.h"
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
	const UAgCombatRules& Rules = UAgSettings::Get()->GetCombatRules();
	if (Rules.HitstopTime <= 0.f)
	{
		return;
	}

	// 캐릭터마다 따로 셉니다. 다른 캐릭터의 적중이 이미 멈춘 캐릭터의 히트스톱을 늘리지 않습니다.
	for (AActor* Actor : Actors)
	{
		if (Actor)
		{
			Actor->CustomTimeDilation = HitstopTimeDilation;
			FTimerHandle& Timer = HitstopTimers.FindOrAdd(Actor);
			GetWorld()->GetTimerManager().SetTimer(Timer, FTimerDelegate::CreateUObject(this, &ThisClass::EndHitstop, TWeakObjectPtr<AActor>(Actor)), Rules.HitstopTime, false);
		}
	}
}

void UAgTimeSubsystem::EndHitstop(TWeakObjectPtr<AActor> Actor)
{
	HitstopTimers.Remove(Actor);
	if (Actor.IsValid())
	{
		Actor->CustomTimeDilation = 1.f;
	}
}

void UAgTimeSubsystem::StartSlowMotion(float Speed, float RealDuration)
{
	if (RealDuration <= 0.f)
	{
		return;
	}
	// Global time dilation slows everything that runs on game time: motion, movement, timers, camera shakes, the HUD's
	// HP trails and the damage numbers. The end is counted in real time.
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
