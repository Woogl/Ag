// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgEffect_Cost.h"

#include "AbilitySystem/AgCostExecution.h"

UAgEffect_Cost::UAgEffect_Cost()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayEffectExecutionDefinition& Execution = Executions.AddDefaulted_GetRef();
	Execution.CalculationClass = UAgCostExecution::StaticClass();
}
