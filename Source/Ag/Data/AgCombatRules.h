// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AgCombatRules.generated.h"

UCLASS(BlueprintType)
class UAgCombatRules : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, VisibleAnywhere, Category = "Hit")
	float GuardReduction = 1.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Hit")
	float DefenseConstant = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "HUD|Trail", meta = (Units = "s"))
	float HUDTrailDelay = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "HUD|Trail", meta = (Units = "s"))
	float HUDTrailShrinkTime = 0.f;
};
