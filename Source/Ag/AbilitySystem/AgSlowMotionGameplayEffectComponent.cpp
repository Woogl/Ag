// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgSlowMotionGameplayEffectComponent.h"

#include "AbilitySystemComponent.h"
#include "Combat/AgTimeSubsystem.h"

void UAgSlowMotionGameplayEffectComponent::OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
	ActiveGEContainer.Owner->GetWorld()->GetSubsystem<UAgTimeSubsystem>()->StartSlowMotion(Speed, Duration);
}
