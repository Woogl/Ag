// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_Teleport.generated.h"

USTRUCT()
struct FAgTeleportParams : public FAgTaskParams
{
	GENERATED_BODY()

	/** 나타날 때 대상과의 캡슐 중심 사이 수평 거리입니다. */
	UPROPERTY()
	float Distance = 0.f;

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

UCLASS()
class UAgAbilityTask_Teleport : public UAbilityTask
{
	GENERATED_BODY()
	friend struct FAgTeleportParams;

public:
	virtual void Activate() override;

private:
	UPROPERTY()
	FAgTeleportParams Params;
};
