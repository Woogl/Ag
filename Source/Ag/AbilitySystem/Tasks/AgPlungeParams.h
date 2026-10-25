// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/AgTaskParams.h"
#include "AgPlungeParams.generated.h"

USTRUCT()
struct FAgPlungeParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	float Speed = 0.f;

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};
