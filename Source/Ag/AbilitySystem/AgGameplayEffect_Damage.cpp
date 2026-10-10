// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgGameplayEffect_Damage.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "Core/AgGameplayTags.h"

UAgGameplayEffect_Damage::UAgGameplayEffect_Damage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	auto AddSetByCallerModifier = [this](const FGameplayAttribute& Attribute, const FGameplayTag& DataTag)
	{
		FSetByCallerFloat SetByCaller;
		SetByCaller.DataTag = DataTag;

		FGameplayModifierInfo& Modifier = Modifiers.AddDefaulted_GetRef();
		Modifier.Attribute = Attribute;
		Modifier.ModifierOp = EGameplayModOp::Additive;
		Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
	};

	AddSetByCallerModifier(UAgAttributeSet::GetHPAttribute(), AgGameplayTags::Data_HP);
	AddSetByCallerModifier(UAgAttributeSet::GetPPAttribute(), AgGameplayTags::Data_PP);
}
