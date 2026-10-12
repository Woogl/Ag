// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgEffect_Resource.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "Core/AgGameplayTags.h"

UAgEffect_Resource::UAgEffect_Resource()
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

	AddSetByCallerModifier(UAgAttributeSet::GetSPAttribute(), AgGameplayTags::SetByCaller_SP);
	AddSetByCallerModifier(UAgAttributeSet::GetMPAttribute(), AgGameplayTags::SetByCaller_MP);
	AddSetByCallerModifier(UAgAttributeSet::GetUPAttribute(), AgGameplayTags::SetByCaller_UP);
}

void UAgEffect_Resource::SetAmounts(FGameplayEffectSpec& Spec, float SP, float MP, float UP)
{
	Spec.SetSetByCallerMagnitude(AgGameplayTags::SetByCaller_SP, SP);
	Spec.SetSetByCallerMagnitude(AgGameplayTags::SetByCaller_MP, MP);
	Spec.SetSetByCallerMagnitude(AgGameplayTags::SetByCaller_UP, UP);
}
