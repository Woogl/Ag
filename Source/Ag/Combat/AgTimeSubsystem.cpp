// Copyright Woogle. All Rights Reserved.

#include "Combat/AgTimeSubsystem.h"

#include "Ag.h"
#include "AbilitySystemComponent.h"
#include "Character/AgCharacterBase.h"
#include "Core/AgGameplayTags.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "TimerManager.h"

namespace
{
	// 엔진의 최솟값(MinGlobalTimeDilation 0.0001)보다 커야 하고, 약 4초의 정지 동안 멈춘 쪽이 0.004초만큼 움직이는
	// 정도라 보이지 않습니다.
	constexpr float TimeStopDilation = 1.e-3f;
}

void UAgTimeSubsystem::StartSlowMotion(float Speed, float RealDuration)
{
	if (RealDuration <= 0.f)
	{
		return;
	}
	// 겹치면 더 느린 속도와 더 긴 남은 시간을 고르므로, 같은 프레임의 요청(궁극기 마무리로 보스 처치)도 처리 순서와
	// 관계없이 결과가 같습니다. 시간 정지 중에는 게임 전체 시간 배율과 타이머를 건드리지 않고 기억만 했다가, 시간
	// 정지가 끝나는 순간 시작합니다.
	if (bTimeStopped)
	{
		if (PendingSlowMotionDuration > 0.f)
		{
			Speed = FMath::Min(Speed, PendingSlowMotionSpeed);
			RealDuration = FMath::Max(RealDuration, PendingSlowMotionDuration);
		}
		PendingSlowMotionSpeed = Speed;
		PendingSlowMotionDuration = RealDuration;
		UE_LOG(LogAg, Verbose, TEXT("Slow motion waits for the time stop to end: speed %.2f for %.2f s of real time"), Speed,
			RealDuration);
		return;
	}
	const FTimerManager& TimerManager = GetWorld()->GetTimerManager();
	if (TimerManager.IsTimerActive(SlowMotionTimer))
	{
		// 타이머는 게임 시간으로 남아 있어 지금 배율로 나눠 실제 남은 시간으로 바꿉니다.
		const AWorldSettings* WorldSettings = GetWorld()->GetWorldSettings();
		Speed = FMath::Min(Speed, WorldSettings->TimeDilation);
		RealDuration = FMath::Max(RealDuration, TimerManager.GetTimerRemaining(SlowMotionTimer) / WorldSettings->GetEffectiveTimeDilation());
	}
	ApplySlowMotion(Speed, RealDuration);
}

void UAgTimeSubsystem::StartTimeStop(AAgCharacterBase& Requester)
{
	if (bTimeStopped)
	{
		UE_LOG(LogAg, Error, TEXT("Time stop is already owned by %s; rejected %s"), *GetNameSafe(TimeStopRequester.Get()), *Requester.GetName());
		return;
	}
	// 정지 중에는 히트스톱 타이머도 멈추므로, 계속 움직일 요청자의 기존 히트스톱을 먼저 끝냅니다.
	Requester.GetAbilitySystemComponent()->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(AgGameplayTags::State_Hitstop));
	// 시간 정지가 게임 전체 시간 배율을 대신 쥐므로, 도는 슬로우모션은 여기서 끝납니다. 지난 시간 정지에서
	// 기다리던 슬로우모션도 이 시간 정지의 끝에 남지 않게 지웁니다.
	GetWorld()->GetTimerManager().ClearTimer(SlowMotionTimer);
	SlowMotionDuration = 0.f;
	PendingSlowMotionDuration = 0.f;
	GetWorld()->GetWorldSettings()->SetTimeDilation(TimeStopDilation);
	bTimeStopped = true;
	TimeStopRequester = &Requester;
	Requester.SetTimeCompensation(GetTimeStopCompensation());
	UE_LOG(LogAg, Verbose, TEXT("Time stop starts: %s keeps moving"), *Requester.GetName());
}

void UAgTimeSubsystem::EndTimeStop(AAgCharacterBase& Requester)
{
	if (!bTimeStopped || TimeStopRequester != &Requester)
	{
		return;
	}
	bTimeStopped = false;
	Requester.SetTimeCompensation(1.f);
	TimeStopRequester.Reset();
	UE_LOG(LogAg, Verbose, TEXT("Time stop ends"));

	if (PendingSlowMotionDuration > 0.f)
	{
		const float Duration = PendingSlowMotionDuration;
		PendingSlowMotionDuration = 0.f;
		ApplySlowMotion(PendingSlowMotionSpeed, Duration);
	}
	else
	{
		GetWorld()->GetWorldSettings()->SetTimeDilation(1.f);
	}
}

float UAgTimeSubsystem::GetTimeStopCompensation() const
{
	return bTimeStopped ? 1.f / GetWorld()->GetWorldSettings()->GetEffectiveTimeDilation() : 1.f;
}

void UAgTimeSubsystem::ApplySlowMotion(float Speed, float RealDuration)
{
	// 배율이 걸린 동안 게임 시간은 실제 시간에 배율을 곱한 만큼 흐르므로, 게임 시간 타이머를 RealDuration × 배율로 걸면
	// 실제 시간 RealDuration 뒤에 끝납니다.
	AWorldSettings* WorldSettings = GetWorld()->GetWorldSettings();
	WorldSettings->SetTimeDilation(Speed);
	SlowMotionDuration = RealDuration;
	GetWorld()->GetTimerManager().SetTimer(SlowMotionTimer, this, &ThisClass::EndSlowMotion, RealDuration * WorldSettings->GetEffectiveTimeDilation(), false);
	UE_LOG(LogAg, Verbose, TEXT("Slow motion starts: speed %.2f for %.2f s of real time"), Speed, RealDuration);
}

void UAgTimeSubsystem::EndSlowMotion()
{
	SlowMotionDuration = 0.f;
	GetWorld()->GetWorldSettings()->SetTimeDilation(1.f);
	UE_LOG(LogAg, Verbose, TEXT("Slow motion ends"));
}
