// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgEffect_PPRefill.h"

#include "AbilitySystem/AgAttributeSet.h"

UAgEffect_PPRefill::UAgEffect_PPRefill()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FAttributeBasedFloat MaxPP;
	MaxPP.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(UAgAttributeSet::GetMaxPPAttribute(), EGameplayEffectAttributeCaptureSource::Target, /*bSnapshot*/ false);

	FGameplayModifierInfo& Modifier = Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = UAgAttributeSet::GetPPAttribute();
	Modifier.ModifierOp = EGameplayModOp::Override;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(MaxPP);
}
