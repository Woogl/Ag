// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgEffect_ParryPPLoss.h"

#include "AbilitySystem/AgParryPPLossExecution.h"

UAgEffect_ParryPPLoss::UAgEffect_ParryPPLoss()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayEffectExecutionDefinition& Execution = Executions.AddDefaulted_GetRef();
	Execution.CalculationClass = UAgParryPPLossExecution::StaticClass();
}
