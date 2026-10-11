// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "AgCostExecution.generated.h"

/** 행동 비용 GE의 계산: 비용을 치르는 어빌리티(GE 컨텍스트의 어빌리티)의 GetCost만큼 SP·MP·UP를 줄입니다. */
UCLASS()
class UAgCostExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
