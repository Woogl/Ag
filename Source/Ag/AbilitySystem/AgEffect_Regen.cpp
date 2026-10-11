// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgEffect_Regen.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/AgRegenCalculation.h"
#include "AbilitySystemComponent.h"
#include "Character/AgCharacterBase.h"
#include "Core/AgGameplayTags.h"
#include "Core/AgSettings.h"
#include "Data/AgCharacterData.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"

UAgEffect_Regen::UAgEffect_Regen()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	// 실제 주기는 걸 때 스펙에 정합니다(Apply). 이 값은 자리만 채웁니다.
	Period = FScalableFloat(1.f);
	bExecutePeriodicEffectOnApplication = false;
	PeriodicInhibitionPolicy = EGameplayEffectPeriodInhibitionRemovedPolicy::ResetPeriod;
}

void UAgEffect_Regen::Apply(UAbilitySystemComponent& ASC, TSubclassOf<UAgEffect_Regen> RegenClass)
{
	const FGameplayEffectSpecHandle Spec = ASC.MakeOutgoingSpec(RegenClass, 1.f, ASC.MakeEffectContext());
	Spec.Data->Period = UAgSettings::Get()->RegenTickInterval;
	ASC.ApplyGameplayEffectSpecToSelf(*Spec.Data);
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
