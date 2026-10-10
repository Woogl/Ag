// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgGameplayEffect_Resource.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "Core/AgGameplayTags.h"

UAgGameplayEffect_Resource::UAgGameplayEffect_Resource()
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

	AddSetByCallerModifier(UAgAttributeSet::GetSPAttribute(), AgGameplayTags::Data_SP);
	AddSetByCallerModifier(UAgAttributeSet::GetMPAttribute(), AgGameplayTags::Data_MP);
	AddSetByCallerModifier(UAgAttributeSet::GetUPAttribute(), AgGameplayTags::Data_UP);
}
