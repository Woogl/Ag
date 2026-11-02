// Copyright Woogle. All Rights Reserved.

#include "Data/AgAbilityData.h"
#include "Data/AgEffectData.h"

bool FAgResourceCost::IsFree() const
{
	return SP <= 0.f && MP <= 0.f && UP <= 0.f;
}

void FAgResourceCost::AddToSpec(FGameplayEffectSpec& Spec) const
{
	AgEffectData::SetResourceAmounts(Spec, -SP, -MP, -UP);
}

bool FAgPatternSelection::IsAvailable(float TargetDistance, int32 Phase) const
{
	return Distance.Contains(TargetDistance) && (Phases.IsEmpty() || Phases.Contains(Phase));
}

bool FAgAbilityData::HasSameGrantSettings(const FAgAbilityData& Other) const
{
	if (AbilityTags != Other.AbilityTags || ActivationRequiredTags != Other.ActivationRequiredTags ||
		ActivationBlockedTags != Other.ActivationBlockedTags || ActivationOwnedTags != Other.ActivationOwnedTags ||
		BlockAbilitiesWithTag != Other.BlockAbilitiesWithTag || CancelAbilitiesWithTag != Other.CancelAbilitiesWithTag ||
		bAllowRetrigger != Other.bAllowRetrigger || AbilityTriggers.Num() != Other.AbilityTriggers.Num())
	{
		return false;
	}
	for (int32 Index = 0; Index < AbilityTriggers.Num(); ++Index)
	{
		if (AbilityTriggers[Index].TriggerTag != Other.AbilityTriggers[Index].TriggerTag ||
			AbilityTriggers[Index].TriggerSource != Other.AbilityTriggers[Index].TriggerSource)
		{
			return false;
		}
	}
	return true;
}
