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

	/** 나타날 때 행동 대상과의 캡슐 중심 사이 수평 거리입니다. */
	UPROPERTY()
	float Distance = 0.f;

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

/** 행동 대상의 등 뒤로 순간이동합니다. 나타날 수 있는 자리가 없으면 제자리에 다시 나타납니다. */
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
