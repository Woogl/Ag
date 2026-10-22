// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Camera/AgCameraShake.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_CameraShake.generated.h"

USTRUCT()
struct FAgCameraShakeParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	EAgCameraShake Shake = EAgCameraShake::None;

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

UCLASS()
class UAgAbilityTask_CameraShake : public UAbilityTask
{
	GENERATED_BODY()
	friend struct FAgCameraShakeParams;

public:
	virtual void Activate() override;

private:
	UPROPERTY()
	FAgCameraShakeParams Params;
};
