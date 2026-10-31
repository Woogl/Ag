// Copyright Woogle. All Rights Reserved.

#include "Data/AgHitReward.h"
#include "Data/AgEffectData.h"

void FAgHitReward::AddToSpec(FGameplayEffectSpec& Spec) const
{
	AgEffectData::SetResourceAmounts(Spec, 0.f, MP, UP);
}
