// Copyright Woogle. All Rights Reserved.

#include "Combat/AgTimeSubsystem.h"

#include "Ag.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "TimerManager.h"

void UAgTimeSubsystem::StartSlowMotion(float Speed, float RealDuration)
{
	if (RealDuration <= 0.f)
	{
		return;
	}
	// 게임 전체 시간 배율은 게임 시간으로 흐르는 모든 것을 늦춥니다: 모션, 이동, 타이머, 카메라 셰이크, HUD의 HP 잔상.
	// 그동안 게임 시간은 실제 시간에 배율을 곱한 만큼 흐르므로, 게임 시간 타이머를 RealDuration × 배율로 걸면 실제 시간 RealDuration 뒤에 끝납니다.
	AWorldSettings* WorldSettings = GetWorld()->GetWorldSettings();
	WorldSettings->SetTimeDilation(Speed);
	GetWorld()->GetTimerManager().SetTimer(SlowMotionTimer, this, &ThisClass::EndSlowMotion, RealDuration * WorldSettings->GetEffectiveTimeDilation(), false);
	UE_LOG(LogAg, Verbose, TEXT("Slow motion starts: speed %.2f for %.2f s of real time"), Speed, RealDuration);
}

void UAgTimeSubsystem::EndSlowMotion()
{
	GetWorld()->GetWorldSettings()->SetTimeDilation(1.f);
	UE_LOG(LogAg, Verbose, TEXT("Slow motion ends"));
}
