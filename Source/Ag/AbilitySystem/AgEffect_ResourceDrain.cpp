// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgEffect_ResourceDrain.h"

UAgEffect_ResourceDrain::UAgEffect_ResourceDrain()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(0.1f);
	bExecutePeriodicEffectOnApplication = false;
}
