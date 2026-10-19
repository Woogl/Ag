// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgParryPPLossExecution.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Character/AgCharacterBase.h"
#include "Data/AgCharacterData.h"

namespace
{
	const FGameplayEffectAttributeCaptureDefinition& GetMaxPPCapture()
	{
		static const FGameplayEffectAttributeCaptureDefinition MaxPP(UAgAttributeSet::GetMaxPPAttribute(), EGameplayEffectAttributeCaptureSource::Target, false);
		return MaxPP;
	}
}

UAgParryPPLossExecution::UAgParryPPLossExecution()
{
	RelevantAttributesToCapture.Add(GetMaxPPCapture());
}

void UAgParryPPLossExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const AAgCharacterBase* Target = CastChecked<AAgCharacterBase>(ExecutionParams.GetTargetAbilitySystemComponent()->GetAvatarActor());
	if (!Target->CanLosePP())
	{
		return;
	}

	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvaluateParameters.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	float MaxPP = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetMaxPPCapture(), EvaluateParameters, MaxPP);

	const AAgCharacterBase* Parrier = CastChecked<AAgCharacterBase>(Spec.GetContext().GetInstigator());
	const float PPLoss = FMath::RoundToFloat(MaxPP * CastChecked<UAgPlayerData>(Parrier->GetCharacterData())->ParryPPRatio);
	if (PPLoss > 0.f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UAgAttributeSet::GetPPAttribute(), EGameplayModOp::Additive, -PPLoss));
	}
}
