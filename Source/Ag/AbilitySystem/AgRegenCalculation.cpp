// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgRegenCalculation.h"

#include "AbilitySystem/AgEffect_Regen.h"
#include "Character/AgCharacterBase.h"

float UAgRegenCalculation::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	// 리젠은 주인이 자기에게 거는 효과라, 컨텍스트의 Instigator가 주인입니다.
	const UAgEffect_Regen* Regen = CastChecked<UAgEffect_Regen>(Spec.Def);
	const AAgCharacterBase* Owner = CastChecked<AAgCharacterBase>(Spec.GetContext().GetInstigator());
	return Regen->GetRatePerSecond(*Owner) * Spec.GetPeriod();
}
