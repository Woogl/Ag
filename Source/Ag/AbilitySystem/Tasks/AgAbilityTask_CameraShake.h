// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgCameraData.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_CameraShake.generated.h"

USTRUCT()
struct FAgCameraShakeParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	EAgCameraShake Shake = EAgCameraShake::None;

	UPROPERTY()
	float Range = 0.f;

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

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
