// Copyright Woogle. All Rights Reserved.
#include "AbilitySystem/AgEffect_Cost.h"
#include "Data/AgAbilityTaskParams.h"

void UAgEffect_Cost::SetCost(FGameplayEffectSpec& Spec, const FAgResourceCostParams& Cost)
{
	float SP = 0.f;
	float MP = 0.f;
	float UP = 0.f;
	switch (Cost.Resource)
	{
	case EAgResource::SP:
		SP = -Cost.Amount;
		break;
	case EAgResource::MP:
		MP = -Cost.Amount;
		break;
	case EAgResource::UP:
		UP = -Cost.Amount;
		break;
	}
	SetAmounts(Spec, SP, MP, UP);
}
