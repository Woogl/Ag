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
	return Tags == Other.Tags && FAgAbilityActivation::StaticStruct()->CompareScriptStruct(&Activation, &Other.Activation, 0);
}
