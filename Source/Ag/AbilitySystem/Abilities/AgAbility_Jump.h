// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_Jump.generated.h"

/**
 * 점프: the character movement's jump (its stock height and air control). No other action starts in the air, but
 * movement input steers. A sprint keeps going through the jump. Without movement input the take-off and landing
 * motions play; while moving, the airborne motion covers the jump.
 */
UCLASS()
class UAgAbility_Jump : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgAbility_Jump();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	UFUNCTION()
	void HandleLanded(EMovementMode NewMovementMode);

	/** True while there is movement input. */
	bool IsMoving() const;
};
