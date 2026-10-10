// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgAttributeSet.h"

#include "GameplayEffectExtension.h"

void UAgAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	ClampToRange(Attribute, NewValue);
}

void UAgAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// Instant effects change the base value, which PreAttributeChange doesn't see.
	float Value = Data.EvaluatedData.Attribute.GetNumericValue(this);
	ClampToRange(Data.EvaluatedData.Attribute, Value);
	Data.EvaluatedData.Attribute.SetNumericValueChecked(Value, this);
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
