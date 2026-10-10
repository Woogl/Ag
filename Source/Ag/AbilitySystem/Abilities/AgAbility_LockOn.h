// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_LockOn.generated.h"

/**
 * 락온 (플레이어 사양 '락온'): runs while the player is locked on. The lock-on key starts it and, while it runs, ends it.
 * It isn't an action: the action state doesn't limit it and no action or reaction cancels it (플레이어 입력 '공통 입력 처리').
 * The lock-on component picks and holds the target; the lock-on task turns the camera and ends this by the release conditions.
 */
UCLASS()
class UAgAbility_LockOn : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgAbility_LockOn();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	/** The player pressed the lock-on key again (sent by the player character while this ability runs). */
	UFUNCTION()
	void HandleLockOnInput(FGameplayEventData Payload);

	UFUNCTION()
	void HandleTargetLost();
};
