// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectComponent.h"
#include "AgDamageResultGameplayEffectComponent.generated.h"

UCLASS(DisplayName = "Ag Damage Result")
class UAgDamageResultGameplayEffectComponent : public UGameplayEffectComponent
{
	GENERATED_BODY()

public:
	virtual void OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const override;
};
