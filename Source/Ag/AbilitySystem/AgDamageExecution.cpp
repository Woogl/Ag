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
	/** 공격자의 ATK는 스펙을 만들 때 값으로, 대상의 DEF와 SP는 적용할 때 값으로 읽습니다. */
	struct FAgDamageCaptures
	{
		FGameplayEffectAttributeCaptureDefinition ATK;
		FGameplayEffectAttributeCaptureDefinition DEF;
		FGameplayEffectAttributeCaptureDefinition SP;

		FAgDamageCaptures()
			: ATK(UAgAttributeSet::GetATKAttribute(), EGameplayEffectAttributeCaptureSource::Source, /*bSnapshot*/ true)
			, DEF(UAgAttributeSet::GetDEFAttribute(), EGameplayEffectAttributeCaptureSource::Target, /*bSnapshot*/ false)
			, SP(UAgAttributeSet::GetSPAttribute(), EGameplayEffectAttributeCaptureSource::Target, /*bSnapshot*/ false)
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
}

UAgDamageExecution::UAgDamageExecution()
{
	RelevantAttributesToCapture.Add(GetCaptures().ATK);
	RelevantAttributesToCapture.Add(GetCaptures().DEF);
	RelevantAttributesToCapture.Add(GetCaptures().SP);
}

int32 UAgDamageExecution::CalculateHPDamage(float SourceATK, float DamageMultiplier, float TargetDEF, float DefenseConstant, float GuardReduction)
{
	return FMath::RoundToInt(SourceATK * DamageMultiplier * (DefenseConstant / (DefenseConstant + TargetDEF)) * GuardReduction);
}

int32 UAgDamageExecution::CalculatePPDamage(float SourceATK, float PoiseMultiplier, float TargetDEF, float DefenseConstant)
{
	return FMath::RoundToInt(SourceATK * PoiseMultiplier * (DefenseConstant / (DefenseConstant + TargetDEF)));
}

void UAgDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	FGameplayEffectSpec& Spec = *ExecutionParams.GetOwningSpecForPreExecuteMod();
	const FGameplayTagContainer& TargetTags = Spec.CapturedTargetTags.GetActorTags();
	const FGameplayTagContainer& AttackTags = Spec.GetDynamicAssetTags();

	// 극한 회피와 무적에는 아무것도 바뀌지 않습니다. 극한 회피의 보상은 회피 어빌리티가 줍니다.
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
	const bool bGuardable = !AttackTags.HasTagExact(AgGameplayTags::Effect_Damage_Unguardable);
	const bool bParryable = !AttackTags.HasTagExact(AgGameplayTags::Effect_Damage_Unparryable);
	const float DamageMultiplier = Spec.GetSetByCallerMagnitude(AgGameplayTags::Data_DamageMultiplier);
	const int32 HPDamage = CalculateHPDamage(SourceATK, DamageMultiplier, TargetDEF, Rules.DefenseConstant);

	// 패리: 막는 쪽은 아무것도 잃지 않습니다. 공격한 쪽의 PP는 패리 이벤트를 받은 가드 어빌리티가 줄입니다.
	if (TargetTags.HasTag(AgGameplayTags::State_ParryWindow) && bGuardable && bParryable && DefenderData)
	{
		Spec.AddDynamicAssetTag(AgGameplayTags::Effect_Damage_Result_Parry);
		return;
	}

	// 가드 또는 가드 브레이크: HP 감소량에 가드 경감 배율을 곱하고 PP는 줄지 않으며, 줄기 전 HP 감소량에 비례한 SP로 치릅니다.
	// SP가 모자라면 가드 브레이크가 되어 SP가 0이 됩니다.
	if (TargetTags.HasTag(AgGameplayTags::State_Guarding) && bGuardable && DefenderData)
	{
		const float GuardSPCost = HPDamage * DefenderData->GuardSPCostRatio;
		const bool bGuardBreak = TargetSP < GuardSPCost;
		Spec.AddDynamicAssetTag(bGuardBreak ? AgGameplayTags::Effect_Damage_Result_GuardBreak : AgGameplayTags::Effect_Damage_Result_Guard);
		AddOutput(OutExecutionOutput, UAgAttributeSet::GetHPAttribute(), -CalculateHPDamage(SourceATK, DamageMultiplier, TargetDEF, Rules.DefenseConstant, Rules.GuardReduction));
		AddOutput(OutExecutionOutput, UAgAttributeSet::GetSPAttribute(), bGuardBreak ? -TargetSP : -GuardSPCost);
		return;
	}

	// 일반 피격: HP를 먼저 낮춥니다. 사망 판정이 그로기 판정보다 먼저입니다(상태 우선순위).
	Spec.AddDynamicAssetTag(AgGameplayTags::Effect_Damage_Result_Hit);
	const int32 PPDamage = Target->CanLosePP() ? CalculatePPDamage(SourceATK, Spec.GetSetByCallerMagnitude(AgGameplayTags::Data_PoiseMultiplier), TargetDEF, Rules.DefenseConstant) : 0;
	AddOutput(OutExecutionOutput, UAgAttributeSet::GetHPAttribute(), -HPDamage);
	AddOutput(OutExecutionOutput, UAgAttributeSet::GetPPAttribute(), -PPDamage);
}
