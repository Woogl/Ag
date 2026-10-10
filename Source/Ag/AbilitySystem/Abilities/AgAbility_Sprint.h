// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_Sprint.generated.h"

/**
 * 달리기: starts when a dodge motion ends with Shift and movement held, and runs until the player character cancels it
 * (Shift released, no movement input, SP at 0) or another action starts. While it runs, State.Sprinting raises the
 * speed, turns the character toward its movement and lets the sprint SP cost effect run.
 */
UCLASS()
class UAgAbility_Sprint : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgAbility_Sprint();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual bool CanPayCost(const UAgAttributeSet& Stats) const override;
};
