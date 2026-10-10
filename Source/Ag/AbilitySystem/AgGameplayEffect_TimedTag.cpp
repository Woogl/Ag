// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgGameplayEffect_TimedTag.h"

UAgGameplayEffect_TimedTag::UAgGameplayEffect_TimedTag()
{
	// The real duration is set on each spec; this is only a valid placeholder.
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.f));
}
