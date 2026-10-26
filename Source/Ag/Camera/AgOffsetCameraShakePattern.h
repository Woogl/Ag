// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Shakes/SimpleCameraShakePattern.h"
#include "AgOffsetCameraShakePattern.generated.h"

/** 시야각을 들어가기(BlendInTime) → 유지 → 돌아오기(BlendOutTime)로 더합니다. Duration은 세 시간의 합입니다. */
UCLASS()
class UAgOffsetCameraShakePattern : public USimpleCameraShakePattern
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Offset")
	float FieldOfView = 0.f;

protected:
	virtual void UpdateShakePatternImpl(const FCameraShakePatternUpdateParams& Params, FCameraShakePatternUpdateResult& OutResult) override;
	virtual void ScrubShakePatternImpl(const FCameraShakePatternScrubParams& Params, FCameraShakePatternUpdateResult& OutResult) override;

private:
	void ApplyOffset(float BlendWeight, FCameraShakePatternUpdateResult& OutResult) const;
};
