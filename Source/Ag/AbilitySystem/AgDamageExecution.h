// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "AgDamageExecution.generated.h"

UCLASS()
class UAgDamageExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UAgDamageExecution();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
