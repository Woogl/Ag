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

void AgResource::SetAmount(FGameplayEffectSpec& Spec, EAgResource Resource, float Amount)
{
	SetAmounts(Spec, Resource == EAgResource::SP ? Amount : 0.f, Resource == EAgResource::MP ? Amount : 0.f,
		Resource == EAgResource::UP ? Amount : 0.f);
}

void FAgResourceCostParams::AddToSpec(FGameplayEffectSpec& Spec) const
{
	AgResource::SetAmount(Spec, Resource, -Amount);
}

bool FAgAbilityData::HasSameGrantSettings(const FAgAbilityData& Other) const
{
	return Tags == Other.Tags && FAgAbilityActivation::StaticStruct()->CompareScriptStruct(&Activation, &Other.Activation, 0);
}
