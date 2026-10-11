// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgEffect_Kill.h"

#include "AbilitySystem/AgAttributeSet.h"

UAgEffect_Kill::UAgEffect_Kill()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo& Modifier = Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = UAgAttributeSet::GetHPAttribute();
	Modifier.ModifierOp = EGameplayModOp::Override;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.f));
}
