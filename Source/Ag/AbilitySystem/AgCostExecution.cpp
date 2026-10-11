// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgCostExecution.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/AgGameplayAbility.h"

void UAgCostExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const UAgGameplayAbility* Ability = CastChecked<UAgGameplayAbility>(ExecutionParams.GetOwningSpec().GetContext().GetAbilityInstance_NotReplicated());
	const FAgAbilityCost Cost = Ability->GetCost();
	const TPair<FGameplayAttribute, float> Spends[] = {
		{ UAgAttributeSet::GetSPAttribute(), Cost.SP },
		{ UAgAttributeSet::GetMPAttribute(), Cost.MP },
		{ UAgAttributeSet::GetUPAttribute(), Cost.UP },
	};
	for (const TPair<FGameplayAttribute, float>& Spend : Spends)
	{
		if (Spend.Value > 0.f)
		{
			OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(Spend.Key, EGameplayModOp::Additive, -Spend.Value));
		}
	}
}
