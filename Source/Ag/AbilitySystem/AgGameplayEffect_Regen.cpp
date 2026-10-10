// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgGameplayEffect_Regen.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "Core/AgGameplayTags.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"

UAgGameplayEffect_Regen::UAgGameplayEffect_Regen()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	// The real period is set on each spec; this is only a valid placeholder.
	Period = FScalableFloat(1.f);
	bExecutePeriodicEffectOnApplication = false;
	PeriodicInhibitionPolicy = EGameplayEffectPeriodInhibitionRemovedPolicy::ResetPeriod;
}

void UAgGameplayEffect_Regen::SetupRegen(const FGameplayAttribute& Attribute, const FGameplayTag& DataTag, const FGameplayTagContainer& RequiredTags, const FGameplayTagContainer& IgnoredTags)
{
	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = DataTag;

	FGameplayModifierInfo& Modifier = Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = Attribute;
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

	UTargetTagRequirementsGameplayEffectComponent* Requirements = CreateDefaultSubobject<UTargetTagRequirementsGameplayEffectComponent>(TEXT("TagRequirements"));
	Requirements->OngoingTagRequirements.RequireTags = RequiredTags;
	Requirements->OngoingTagRequirements.IgnoreTags = IgnoredTags;
	GEComponents.Add(Requirements);
}

UAgGameplayEffect_SPRegen::UAgGameplayEffect_SPRegen()
{
	FGameplayTagContainer Ignored;
	Ignored.AddTag(AgGameplayTags::State_Regen_SPDelay);
	Ignored.AddTag(AgGameplayTags::State_Guarding);
	SetupRegen(UAgAttributeSet::GetSPAttribute(), AgGameplayTags::Data_SP, FGameplayTagContainer(), Ignored);
}

UAgGameplayEffect_SPRegenGuarding::UAgGameplayEffect_SPRegenGuarding()
{
	SetupRegen(UAgAttributeSet::GetSPAttribute(), AgGameplayTags::Data_SP, FGameplayTagContainer(AgGameplayTags::State_Guarding), FGameplayTagContainer(AgGameplayTags::State_Regen_SPDelay));
}

UAgGameplayEffect_PPRegen::UAgGameplayEffect_PPRegen()
{
	SetupRegen(UAgAttributeSet::GetPPAttribute(), AgGameplayTags::Data_PP, FGameplayTagContainer(), FGameplayTagContainer(AgGameplayTags::State_Regen_PPDelay));
}
