// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgAttributeSet.h"

void UAgAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	// Instant and periodic effects (damage, costs, regen) change the base value; keep it in range so a regen tick at
	// the maximum doesn't pile up above it and swallow the next decrease.
	ClampToRange(Attribute, NewValue);
}

void UAgAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	ClampToRange(Attribute, NewValue);
}

void UAgAttributeSet::ClampToRange(const FGameplayAttribute& Attribute, float& Value) const
{
	if (Attribute == GetHPAttribute())
	{
		Value = FMath::Clamp(Value, 0.f, GetMaxHP());
	}
	else if (Attribute == GetSPAttribute())
	{
		Value = FMath::Clamp(Value, 0.f, GetMaxSP());
	}
	else if (Attribute == GetMPAttribute())
	{
		Value = FMath::Clamp(Value, 0.f, GetMaxMP());
	}
	else if (Attribute == GetUPAttribute())
	{
		Value = FMath::Clamp(Value, 0.f, GetMaxUP());
	}
	else if (Attribute == GetPPAttribute())
	{
		Value = FMath::Clamp(Value, 0.f, GetMaxPP());
	}
	else
	{
		Value = FMath::Max(Value, 0.f);
	}
}
