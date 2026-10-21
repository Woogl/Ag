// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AgHitReward.generated.h"

USTRUCT()
struct FAgHitReward
{
	GENERATED_BODY()

	UPROPERTY()
	float MP = 0.f;

	UPROPERTY()
	float UP = 0.f;
};
