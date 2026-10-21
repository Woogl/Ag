// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectComponent.h"
#include "AgSlowMotionGameplayEffectComponent.generated.h"

UCLASS()
class UAgSlowMotionGameplayEffectComponent : public UGameplayEffectComponent
{
	GENERATED_BODY()

public:
	virtual void OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const override;

	UPROPERTY()
	float Speed = 1.f;

	UPROPERTY()
	float Duration = 0.f;
};
