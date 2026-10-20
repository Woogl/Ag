// Copyright Woogle. All Rights Reserved.

#include "Data/AgAbilityData.h"
#include "Core/AgGameplayTags.h"
#include "GameplayEffect.h"

void AgResource::SetAmounts(FGameplayEffectSpec& Spec, float SP, float MP, float UP)
{
	Spec.SetSetByCallerMagnitude(AgGameplayTags::SetByCaller_SP, SP);
	Spec.SetSetByCallerMagnitude(AgGameplayTags::SetByCaller_MP, MP);
	Spec.SetSetByCallerMagnitude(AgGameplayTags::SetByCaller_UP, UP);
}

bool FAgResourceCost::IsFree() const
{
	return SP <= 0.f && MP <= 0.f && UP <= 0.f;
}

void FAgResourceCost::AddToSpec(FGameplayEffectSpec& Spec) const
{
	AgResource::SetAmounts(Spec, -SP, -MP, -UP);
}

bool FAgAbilityData::HasSameGrantSettings(const FAgAbilityData& Other) const
{
	return Tags == Other.Tags && FAgAbilityActivation::StaticStruct()->CompareScriptStruct(&Activation, &Other.Activation, 0);
}
