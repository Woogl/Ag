// Copyright Woogle. All Rights Reserved.

#include "Camera/AgOffsetCameraShakePattern.h"

void UAgOffsetCameraShakePattern::UpdateShakePatternImpl(const FCameraShakePatternUpdateParams& Params, FCameraShakePatternUpdateResult& OutResult)
{
	ApplyOffset(State.Update(Params.DeltaTime), OutResult);
}

void UAgOffsetCameraShakePattern::ScrubShakePatternImpl(const FCameraShakePatternScrubParams& Params, FCameraShakePatternUpdateResult& OutResult)
{
	ApplyOffset(State.Scrub(Params.AbsoluteTime), OutResult);
}

void UAgOffsetCameraShakePattern::ApplyOffset(float BlendWeight, FCameraShakePatternUpdateResult& OutResult) const
{
	// 엔진의 블렌드 가중치는 선형이라, 변화가 시작하고 멈추는 순간이 튀지 않게 이즈 인·아웃을 겁니다.
	OutResult.FOV = FieldOfView;
	OutResult.ApplyScale(FMath::InterpEaseInOut(0.f, 1.f, BlendWeight, 2.f));
}
