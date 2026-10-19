// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "AgParryPPLossExecution.generated.h"

UCLASS()
class UAgParryPPLossExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UAgParryPPLossExecution();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
