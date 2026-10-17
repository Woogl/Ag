// Copyright Woogle. All Rights Reserved.

#include "Data/AgAbilityTaskParams.h"
#include "Core/AgGameplayTags.h"
#include "GameplayEffect.h"

namespace
{
	/** 자원 효과의 SetByCaller 셋을 모두 넣습니다. 넣지 않은 값은 엔진이 오류로 남기므로, 바꾸지 않는 자원도 0을 넣습니다. */
	void SetResourceAmounts(FGameplayEffectSpec& Spec, float SP, float MP, float UP)
	{
		Spec.SetSetByCallerMagnitude(AgGameplayTags::SetByCaller_SP, SP);
		Spec.SetSetByCallerMagnitude(AgGameplayTags::SetByCaller_MP, MP);
		Spec.SetSetByCallerMagnitude(AgGameplayTags::SetByCaller_UP, UP);
	}

	void SetResourceAmount(FGameplayEffectSpec& Spec, EAgResource Resource, float Amount)
	{
		SetResourceAmounts(Spec, Resource == EAgResource::SP ? Amount : 0.f, Resource == EAgResource::MP ? Amount : 0.f,
		    Resource == EAgResource::UP ? Amount : 0.f);
	}
}

void FAgResourceCostParams::AddToSpec(FGameplayEffectSpec& Spec) const
{
	SetResourceAmount(Spec, Resource, -Amount);
}

void FAgDrainResourceParams::AddToSpec(FGameplayEffectSpec& Spec) const
{
	SetResourceAmount(Spec, Resource, -AmountPerSecond * Spec.GetPeriod());
}

void FAgGrantResourcesParams::AddToSpec(FGameplayEffectSpec& Spec) const
{
	SetResourceAmounts(Spec, SP, MP, UP);
}
