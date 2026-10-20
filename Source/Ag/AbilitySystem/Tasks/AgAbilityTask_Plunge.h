// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_Plunge.generated.h"

USTRUCT()
struct FAgPlungeParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	float Speed = 0.f;

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};
