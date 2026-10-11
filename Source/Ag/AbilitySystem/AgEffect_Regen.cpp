// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgEffect_Regen.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/AgRegenCalculation.h"
#include "Character/AgCharacterBase.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"

UAgEffect_Regen::UAgEffect_Regen()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	// 리젠을 나눠 적용하는 간격. 기획서에 수치가 없는 구현 값으로, 게이지가 부드럽게 찰 만큼 짧게 둡니다.
	Period = FScalableFloat(0.1f);
	bExecutePeriodicEffectOnApplication = false;
	PeriodicInhibitionPolicy = EGameplayEffectPeriodInhibitionRemovedPolicy::ResetPeriod;
}

void UAgEffect_Regen::SetupRegen(const FGameplayAttribute& Attribute, const FGameplayTagContainer& RequiredTags, const FGameplayTagContainer& IgnoredTags)
{
	FCustomCalculationBasedFloat Calculation;
	Calculation.CalculationClassMagnitude = UAgRegenCalculation::StaticClass();

	FGameplayModifierInfo& Modifier = Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = Attribute;
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(Calculation);

	UTargetTagRequirementsGameplayEffectComponent* Requirements = CreateDefaultSubobject<UTargetTagRequirementsGameplayEffectComponent>(TEXT("TagRequirements"));
	Requirements->OngoingTagRequirements.RequireTags = RequiredTags;
	Requirements->OngoingTagRequirements.IgnoreTags = IgnoredTags;
	GEComponents.Add(Requirements);
}

UAgEffect_SPRegen::UAgEffect_SPRegen()
{
	FGameplayTagContainer Ignored;
	Ignored.AddTag(AgGameplayTags::State_Regen_SPDelay);
	Ignored.AddTag(AgGameplayTags::State_Guarding);
	SetupRegen(UAgAttributeSet::GetSPAttribute(), FGameplayTagContainer(), Ignored);
}

float UAgEffect_SPRegen::GetRatePerSecond(const AAgCharacterBase& Owner) const
{
	return CastChecked<UAgPlayerData>(Owner.GetCharacterData())->SPRegenRate;
}

UAgEffect_SPRegenGuarding::UAgEffect_SPRegenGuarding()
{
	SetupRegen(UAgAttributeSet::GetSPAttribute(), FGameplayTagContainer(AgGameplayTags::State_Guarding), FGameplayTagContainer(AgGameplayTags::State_Regen_SPDelay));
}

float UAgEffect_SPRegenGuarding::GetRatePerSecond(const AAgCharacterBase& Owner) const
{
	return CastChecked<UAgPlayerData>(Owner.GetCharacterData())->SPRegenRateGuarding;
}

UAgEffect_PPRegen::UAgEffect_PPRegen()
{
	SetupRegen(UAgAttributeSet::GetPPAttribute(), FGameplayTagContainer(), FGameplayTagContainer(AgGameplayTags::State_Regen_PPDelay));
}

float UAgEffect_PPRegen::GetRatePerSecond(const AAgCharacterBase& Owner) const
{
	return Owner.GetCharacterData()->PPRegenRate;
}

UAgEffect_SprintSPCost::UAgEffect_SprintSPCost()
{
	SetupRegen(UAgAttributeSet::GetSPAttribute(), FGameplayTagContainer(AgGameplayTags::State_Sprinting), FGameplayTagContainer());
}

float UAgEffect_SprintSPCost::GetRatePerSecond(const AAgCharacterBase& Owner) const
{
	return -CastChecked<UAgPlayerData>(Owner.GetCharacterData())->SprintSPCost;
}
