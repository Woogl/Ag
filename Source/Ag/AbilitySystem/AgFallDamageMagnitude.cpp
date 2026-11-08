// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgFallDamageMagnitude.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "Core/AgAssetManager.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCombatRules.h"

namespace AgFallDamageMagnitudePrivate
{
	const FGameplayEffectAttributeCaptureDefinition& GetMaxHPCapture()
	{
		static const FGameplayEffectAttributeCaptureDefinition Capture(UAgAttributeSet::GetMaxHPAttribute(), EGameplayEffectAttributeCaptureSource::Target, false);
		return Capture;
	}
}

UAgFallDamageMagnitude::UAgFallDamageMagnitude()
{
	RelevantAttributesToCapture.Add(AgFallDamageMagnitudePrivate::GetMaxHPCapture());
}

float UAgFallDamageMagnitude::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const UAgCombatRules& Rules = UAgAssetManager::Get().GetCombatRules();
	const float FallHeight = Spec.GetSetByCallerMagnitude(AgGameplayTags::SetByCaller_FallHeight);
	const float Ratio = FMath::GetMappedRangeValueClamped(FVector2f(Rules.FallDamageStartHeight, Rules.FallDeathHeight), FVector2f(0.f, 1.f), FallHeight);

	float MaxHP = 0.f;
	GetCapturedAttributeMagnitude(AgFallDamageMagnitudePrivate::GetMaxHPCapture(), Spec, FAggregatorEvaluateParameters(), MaxHP);
	return -FMath::RoundToFloat(MaxHP * Ratio);
}
