// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgCameraData.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_CameraShake.generated.h"

/** 카메라 셰이크(CameraShake Task의 Params). 로컬 플레이어와 주인의 수평 거리가 Range 이내면 Shake 단계로 흔듭니다. */
USTRUCT()
struct FAgCameraShakeParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	EAgCameraShake Shake = EAgCameraShake::None;

	/** 캡슐 중심 사이 수평 거리(cm) */
	UPROPERTY()
	float Range = 0.f;

	virtual EAgTaskPlacement GetPlacements() const override
	{
		return EAgTaskPlacement::Point;
	}

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

/** 셰이크를 한 번 재생하고 끝납니다. */
UCLASS()
class UAgAbilityTask_CameraShake : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UAgAbilityTask_CameraShake* CameraShake(UGameplayAbility* Owner, EAgCameraShake Shake, float Range);

	virtual void Activate() override;

private:
	EAgCameraShake Shake = EAgCameraShake::None;
	float Range = 0.f;
};
