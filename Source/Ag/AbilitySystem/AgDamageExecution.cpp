// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgDamageExecution.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Character/AgCharacterBase.h"
#include "Core/AgAssetManager.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "Data/AgCombatRules.h"
#include "KismetAnimationLibrary.h"

namespace AgDamageExecutionPrivate
{
	struct FAgDamageCaptures
	{
		FGameplayEffectAttributeCaptureDefinition ATK;
		FGameplayEffectAttributeCaptureDefinition DEF;

		FAgDamageCaptures()
			: ATK(UAgAttributeSet::GetATKAttribute(), EGameplayEffectAttributeCaptureSource::Source, true)
			, DEF(UAgAttributeSet::GetDEFAttribute(), EGameplayEffectAttributeCaptureSource::Target, false)
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

	bool IsInvincible(const FGameplayTagContainer& TargetTags, const AActor& Target)
	{
		if (!TargetTags.HasTag(AgGameplayTags::State_Invincible))
		{
			return false;
		}
		// 누워 있는 동안의 무적은 하위 태그로만 붙으므로, 부모 태그를 직접 가진 효과가 없으면 그 무적뿐이고 플레이어만 지킵니다.
		if (TargetTags.HasTagExact(AgGameplayTags::State_Invincible))
		{
			return true;
		}
		const AAgCharacterBase* Character = Cast<AAgCharacterBase>(&Target);
		return Character && Character->GetCharacterData()->Team == EAgTeam::Player;
	}

	int32 CalculateDamage(float SourceATK, float Multiplier, float TargetDEF, float DefenseConstant, float Reduction = 1.f)
	{
		return FMath::RoundToInt(SourceATK * Multiplier * (DefenseConstant / (DefenseConstant + TargetDEF)) * Reduction);
	}

	float GetBonusDamageRatio(const FGameplayEffectSpec& Spec, const AActor& Target, const FGameplayTagContainer& TargetTags, const UAgCombatRules& Rules)
	{
		const AAgCharacterBase* Attacker = Cast<AAgCharacterBase>(Spec.GetContext().GetInstigator());
		const AActor* Causer = Spec.GetContext().GetEffectCauser();
		if (!Attacker || !Causer || Attacker->GetCharacterData()->Team != EAgTeam::Player ||
			Spec.GetDynamicAssetTags().HasTagExact(AgGameplayTags::Effect_Damage_Parry) || TargetTags.HasTag(AgGameplayTags::State_Execution))
		{
			return 1.f;
		}

		float Bonus = 0.f;
		if (TargetTags.HasTag(AgGameplayTags::State_Groggy))
		{
			Bonus += Rules.GroggyBonusDamage;
		}
		if (TargetTags.HasTag(AgGameplayTags::State_Airborne))
		{
			Bonus += Rules.AerialBonusDamage;
		}
		const float Angle = UKismetAnimationLibrary::CalculateDirection(Causer->GetActorLocation() - Target.GetActorLocation(), Target.GetActorRotation());
		if (FMath::Abs(Angle) >= 135.f)
		{
			Bonus += Rules.BackAttackBonusDamage;
		}
		return 1.f + Bonus;
	}
} // namespace AgDamageExecutionPrivate

UAgDamageExecution::UAgDamageExecution()
{
	RelevantAttributesToCapture.Add(AgDamageExecutionPrivate::GetCaptures().ATK);
	RelevantAttributesToCapture.Add(AgDamageExecutionPrivate::GetCaptures().DEF);
}

void UAgDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	FGameplayEffectSpec& Spec = *ExecutionParams.GetOwningSpecForPreExecuteMod();
	const FGameplayTagContainer& TargetTags = Spec.CapturedTargetTags.GetActorTags();
	const FGameplayTagContainer& AttackTags = Spec.GetDynamicAssetTags();
	const AActor& Target = *ExecutionParams.GetTargetAbilitySystemComponent()->GetAvatarActor();

	if (TargetTags.HasTag(AgGameplayTags::State_PerfectDodgeWindow))
	{
		Spec.AddDynamicAssetTag(AgGameplayTags::Effect_Damage_Result_PerfectDodge);
		return;
	}
	if (AgDamageExecutionPrivate::IsInvincible(TargetTags, Target))
	{
		return;
	}

	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvaluateParameters.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	float SourceATK = 0.f;
	float TargetDEF = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(AgDamageExecutionPrivate::GetCaptures().ATK, EvaluateParameters, SourceATK);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(AgDamageExecutionPrivate::GetCaptures().DEF, EvaluateParameters, TargetDEF);

	const UAgCombatRules& Rules = UAgAssetManager::Get().GetCombatRules();
	const bool bGuardable = !AttackTags.HasTagExact(AgGameplayTags::Effect_Damage_Unblockable);
	const float HPMultiplier = Spec.GetSetByCallerMagnitude(AgGameplayTags::SetByCaller_HPMultiplier) * AgDamageExecutionPrivate::GetBonusDamageRatio(Spec, Target, TargetTags, Rules);
	const int32 HPDamage = AgDamageExecutionPrivate::CalculateDamage(SourceATK, HPMultiplier, TargetDEF, Rules.DefenseConstant);

	if (TargetTags.HasTag(AgGameplayTags::State_ParryWindow) && bGuardable)
	{
		Spec.AddDynamicAssetTag(AgGameplayTags::Effect_Damage_Result_Parry);
		return;
	}

	if (TargetTags.HasTag(AgGameplayTags::State_Guarding) && bGuardable)
	{
		Spec.AddDynamicAssetTag(AgGameplayTags::Effect_Damage_Result_Guard);
		AgDamageExecutionPrivate::AddOutput(OutExecutionOutput, UAgAttributeSet::GetHPAttribute(), -AgDamageExecutionPrivate::CalculateDamage(SourceATK, HPMultiplier, TargetDEF, Rules.DefenseConstant, Rules.GuardReduction));
		return;
	}

	// 사망 판정이 그로기 판정보다 먼저이도록 HP를 먼저 낮춥니다.
	Spec.AddDynamicAssetTag(AgGameplayTags::Effect_Damage_Result_Hit);
	const int32 PPDamage = AgDamageExecutionPrivate::CalculateDamage(SourceATK, Spec.GetSetByCallerMagnitude(AgGameplayTags::SetByCaller_PPMultiplier), TargetDEF, Rules.DefenseConstant);
	AgDamageExecutionPrivate::AddOutput(OutExecutionOutput, UAgAttributeSet::GetHPAttribute(), -HPDamage);
	AgDamageExecutionPrivate::AddOutput(OutExecutionOutput, UAgAttributeSet::GetPPAttribute(), -PPDamage);
}
