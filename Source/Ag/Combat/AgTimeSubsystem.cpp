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
	// 게임 전체 시간 배율은 게임 시간으로 흐르는 모든 것을 늦춥니다: 모션, 이동, 타이머, 카메라 셰이크, HUD의 감소 잔상.
	// 그동안 게임 시간은 실제 시간에 배율을 곱한 만큼 흐르므로, 게임 시간 타이머를 RealDuration × 배율로 걸면 실제 시간 RealDuration 뒤에 끝납니다.
	AWorldSettings* WorldSettings = GetWorld()->GetWorldSettings();
	WorldSettings->SetTimeDilation(Speed);
	GetWorld()->GetTimerManager().SetTimer(SlowMotionTimer, this, &ThisClass::EndSlowMotion, RealDuration * WorldSettings->GetEffectiveTimeDilation(), false);
	UE_LOG(LogAg, Verbose, TEXT("Slow motion starts: speed %.2f for %.2f s of real time"), Speed, RealDuration);
}

bool UAgTimeSubsystem::StartTimeStop(UObject& Requester, AAgCharacterBase& Exempt)
{
	if (bTimeStopped)
	{
		UE_LOG(LogAg, Error, TEXT("Time stop is already owned by %s; rejected %s"), *GetNameSafe(TimeStopRequester.Get()), *Requester.GetName());
		return false;
	}
	TimeStopRequester = &Requester;
	// 시간 정지가 게임 전체 시간 배율을 대신 쥐므로, 도는 슬로우모션은 여기서 끝납니다.
	GetWorld()->GetTimerManager().ClearTimer(SlowMotionTimer);
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
	GetWorld()->GetWorldSettings()->SetTimeDilation(1.f);
	bTimeStopped = false;
	if (AAgCharacterBase* Exempt = TimeStopExempt.Get())
	{
		Exempt->SetTimeCompensation(1.f);
	}
	TimeStopExempt.Reset();
	TimeStopRequester.Reset();
	OnTimeStopChanged.Broadcast(false);
	UE_LOG(LogAg, Verbose, TEXT("Time stop ends"));
}

float UAgTimeSubsystem::GetTimeStopCompensation() const
{
	return bTimeStopped ? 1.f / GetWorld()->GetWorldSettings()->GetEffectiveTimeDilation() : 1.f;
}

void UAgTimeSubsystem::EndSlowMotion()
{
	GetWorld()->GetWorldSettings()->SetTimeDilation(1.f);
	UE_LOG(LogAg, Verbose, TEXT("Slow motion ends"));
}
