// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgDamageExecution.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Character/AgCharacterBase.h"
#include "Core/AgAssetManager.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "Data/AgCombatRules.h"

namespace
{
	struct FAgDamageCaptures
	{
		FGameplayEffectAttributeCaptureDefinition ATK;
		FGameplayEffectAttributeCaptureDefinition DEF;
		FGameplayEffectAttributeCaptureDefinition SP;

		FAgDamageCaptures()
			: ATK(UAgAttributeSet::GetATKAttribute(), EGameplayEffectAttributeCaptureSource::Source, true)
			, DEF(UAgAttributeSet::GetDEFAttribute(), EGameplayEffectAttributeCaptureSource::Target, false)
			, SP(UAgAttributeSet::GetSPAttribute(), EGameplayEffectAttributeCaptureSource::Target, false)
		{
		}
	};

	const FAgDamageCaptures& GetCaptures()
	{
		static const FAgDamageCaptures Captures;
		return Captures;
	}

	void AddOutput(FGameplayEffectCustomExecutionOutput& Output, const FGameplayAttribute& Attribute, float Change)
	{
		if (Change != 0.f)
		{
			Output.AddOutputModifier(FGameplayModifierEvaluatedData(Attribute, EGameplayModOp::Additive, Change));
		}
	}

	int32 CalculateDamage(float SourceATK, float Multiplier, float TargetDEF, float DefenseConstant, float Reduction = 1.f)
	{
		return FMath::RoundToInt(SourceATK * Multiplier * (DefenseConstant / (DefenseConstant + TargetDEF)) * Reduction);
	}
}

UAgDamageExecution::UAgDamageExecution()
{
	RelevantAttributesToCapture.Add(GetCaptures().ATK);
	RelevantAttributesToCapture.Add(GetCaptures().DEF);
	RelevantAttributesToCapture.Add(GetCaptures().SP);
}

void UAgDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	FGameplayEffectSpec& Spec = *ExecutionParams.GetOwningSpecForPreExecuteMod();
	const FGameplayTagContainer& TargetTags = Spec.CapturedTargetTags.GetActorTags();
	const FGameplayTagContainer& AttackTags = Spec.GetDynamicAssetTags();

	if (TargetTags.HasTag(AgGameplayTags::State_PerfectDodgeWindow))
	{
		Spec.AddDynamicAssetTag(AgGameplayTags::Effect_Damage_Result_PerfectDodge);
		return;
	}
	if (TargetTags.HasTag(AgGameplayTags::State_Invincible))
	{
		return;
	}

	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvaluateParameters.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	float SourceATK = 0.f;
	float TargetDEF = 0.f;
	float TargetSP = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetCaptures().ATK, EvaluateParameters, SourceATK);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetCaptures().DEF, EvaluateParameters, TargetDEF);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetCaptures().SP, EvaluateParameters, TargetSP);

	const UAgCombatRules& Rules = UAgAssetManager::Get().GetCombatRules();
	const AAgCharacterBase* Target = CastChecked<AAgCharacterBase>(ExecutionParams.GetTargetAbilitySystemComponent()->GetAvatarActor());
	const UAgPlayerData* DefenderData = Cast<UAgPlayerData>(Target->GetCharacterData());
	const bool bGuardable = !AttackTags.HasTagExact(AgGameplayTags::Effect_Damage_Unblockable);
	const float HPMultiplier = Spec.GetSetByCallerMagnitude(AgGameplayTags::SetByCaller_HPMultiplier);
	const int32 HPDamage = CalculateDamage(SourceATK, HPMultiplier, TargetDEF, Rules.DefenseConstant);

	if (TargetTags.HasTag(AgGameplayTags::State_ParryWindow) && bGuardable && DefenderData)
	{
		Spec.AddDynamicAssetTag(AgGameplayTags::Effect_Damage_Result_Parry);
		return;
	}

	if (TargetTags.HasTag(AgGameplayTags::State_Guarding) && bGuardable && DefenderData)
	{
		const float GuardSPCost = HPDamage * Rules.GuardSPCostRatio;
		const bool bGuardBreak = TargetSP < GuardSPCost;
		Spec.AddDynamicAssetTag(bGuardBreak ? AgGameplayTags::Effect_Damage_Result_GuardBreak : AgGameplayTags::Effect_Damage_Result_Guard);
		AddOutput(OutExecutionOutput, UAgAttributeSet::GetHPAttribute(), -CalculateDamage(SourceATK, HPMultiplier, TargetDEF, Rules.DefenseConstant, Rules.GuardReduction));
		AddOutput(OutExecutionOutput, UAgAttributeSet::GetSPAttribute(), bGuardBreak ? -TargetSP : -GuardSPCost);
		return;
	}

	// 사망 판정이 그로기 판정보다 먼저이도록 HP를 먼저 낮춥니다.
	Spec.AddDynamicAssetTag(AgGameplayTags::Effect_Damage_Result_Hit);
	const int32 PPDamage = CalculateDamage(SourceATK, Spec.GetSetByCallerMagnitude(AgGameplayTags::SetByCaller_PPMultiplier), TargetDEF, Rules.DefenseConstant);
	AddOutput(OutExecutionOutput, UAgAttributeSet::GetHPAttribute(), -HPDamage);
	AddOutput(OutExecutionOutput, UAgAttributeSet::GetPPAttribute(), -PPDamage);
}
