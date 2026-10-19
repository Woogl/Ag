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

	static int32 CalculateHPDamage(float SourceATK, float DamageMultiplier, float TargetDEF, float DefenseConstant, float GuardReduction = 1.f);

	static int32 CalculatePPDamage(float SourceATK, float PoiseMultiplier, float TargetDEF, float DefenseConstant);

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
