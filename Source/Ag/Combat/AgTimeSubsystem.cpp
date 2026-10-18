// Copyright Woogle. All Rights Reserved.

#include "Combat/AgTimeSubsystem.h"

#include "Ag.h"
#include "Character/AgCharacterBase.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "TimerManager.h"

namespace
{
	// 시간 정지의 게임 전체 시간 배율. 엔진의 최솟값(MinGlobalTimeDilation 0.0001)보다 커야 하고, 약 4초의 정지 동안
	// 멈춘 쪽이 0.004초만큼 움직이는 정도라 보이지 않습니다.
	constexpr float TimeStopDilation = 1.e-3f;
}

void UAgTimeSubsystem::StartSlowMotion(float Speed, float RealDuration)
{
	if (RealDuration <= 0.f)
	{
		return;
	}
	// 시간 정지 중에는 게임 전체 시간 배율과 타이머를 건드리지 않고 기억만 합니다. 같은 프레임의 처리 순서와 상관없이 시간 정지가
	// 끝나는 순간 시작합니다.
	if (bTimeStopped)
	{
		PendingSlowMotionSpeed = Speed;
		PendingSlowMotionDuration = RealDuration;
		UE_LOG(LogAg, Verbose, TEXT("Slow motion waits for the time stop to end: speed %.2f for %.2f s of real time"), Speed,
			RealDuration);
		return;
	}
	ApplySlowMotion(Speed, RealDuration);
}

bool UAgTimeSubsystem::StartTimeStop(UObject& Requester, AAgCharacterBase& Exempt)
{
	if (bTimeStopped)
	{
		UE_LOG(LogAg, Error, TEXT("Time stop is already owned by %s; rejected %s"), *GetNameSafe(TimeStopRequester.Get()), *Requester.GetName());
		return false;
	}
	TimeStopRequester = &Requester;
	// 시간 정지가 게임 전체 시간 배율을 대신 쥐므로, 도는 슬로우모션은 여기서 끝납니다. 지난 시간 정지에서 기다리던 슬로우모션도
	// 이 시간 정지의 끝에 남지 않게 지웁니다.
	GetWorld()->GetTimerManager().ClearTimer(SlowMotionTimer);
	PendingSlowMotionDuration = 0.f;
	GetWorld()->GetWorldSettings()->SetTimeDilation(TimeStopDilation);
	bTimeStopped = true;
	TimeStopExempt = &Exempt;
	Exempt.SetTimeCompensation(GetTimeStopCompensation());
	OnTimeStopChanged.Broadcast(true);
	UE_LOG(LogAg, Verbose, TEXT("Time stop starts: %s keeps moving"), *Exempt.GetName());
	return true;
}

void UAgTimeSubsystem::EndTimeStop(UObject& Requester)
{
	if (!bTimeStopped || TimeStopRequester != &Requester)
	{
		return;
	}
	bTimeStopped = false;
	if (AAgCharacterBase* Exempt = TimeStopExempt.Get())
	{
		Exempt->SetTimeCompensation(1.f);
	}
	TimeStopExempt.Reset();
	TimeStopRequester.Reset();
	UE_LOG(LogAg, Verbose, TEXT("Time stop ends"));

	// 시간 정지 중에 시작한 슬로우모션은 지금부터 그 길이만큼 돕니다.
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
	OnTimeStopChanged.Broadcast(false);
}

float UAgTimeSubsystem::GetTimeStopCompensation() const
{
	return bTimeStopped ? 1.f / GetWorld()->GetWorldSettings()->GetEffectiveTimeDilation() : 1.f;
}

void UAgTimeSubsystem::ApplySlowMotion(float Speed, float RealDuration)
{
	// 게임 전체 시간 배율은 게임 시간으로 흐르는 모든 것을 늦춥니다: 모션, 이동, 타이머, 카메라 셰이크, HUD의 감소 잔상.
	// 그동안 게임 시간은 실제 시간에 배율을 곱한 만큼 흐르므로, 게임 시간 타이머를 RealDuration × 배율로 걸면 실제 시간 RealDuration 뒤에 끝납니다.
	AWorldSettings* WorldSettings = GetWorld()->GetWorldSettings();
	WorldSettings->SetTimeDilation(Speed);
	GetWorld()->GetTimerManager().SetTimer(SlowMotionTimer, this, &ThisClass::EndSlowMotion, RealDuration * WorldSettings->GetEffectiveTimeDilation(), false);
	UE_LOG(LogAg, Verbose, TEXT("Slow motion starts: speed %.2f for %.2f s of real time"), Speed, RealDuration);
}

void UAgTimeSubsystem::EndSlowMotion()
{
	// 시간 정지가 시작될 때 끝 타이머를 지우므로 생기지 않는 경우지만, 시간 정지를 풀지 않도록 막습니다.
	if (bTimeStopped)
	{
		return;
	}
	GetWorld()->GetWorldSettings()->SetTimeDilation(1.f);
	UE_LOG(LogAg, Verbose, TEXT("Slow motion ends"));
}
