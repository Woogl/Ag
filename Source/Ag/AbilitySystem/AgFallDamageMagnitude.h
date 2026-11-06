// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "AgFallDamageMagnitude.generated.h"

UCLASS()
class UAgFallDamageMagnitude : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UAgFallDamageMagnitude();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
