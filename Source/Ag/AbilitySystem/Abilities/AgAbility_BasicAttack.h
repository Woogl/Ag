// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_BasicAttack.generated.h"

class UAbilityTask_PlayMontageAndWait;

/**
 * 평타: a combo of up to four hits, each its own action.
 * Pressing attack during a hit's recovery (후딜) starts the next hit (after the last, the first again). The combo resets
 * when a recovery ends without input, or when another action or a knockback/down cancels it.
 */
UCLASS()
class UAgAbility_BasicAttack : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgAbility_BasicAttack();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	void StartHit(int32 Index);

	/** The player pressed attack again (sent by the player character while this ability runs). */
	UFUNCTION()
	void HandleAttackInput(FGameplayEventData Payload);

	UFUNCTION()
	void HandleAttackWindowEnded(FGameplayEventData Payload);

	UFUNCTION()
	void HandleMontageFinished();

	UFUNCTION()
	void HandleMontageCancelled();

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	int32 HitIndex = 0;
	bool bInRecovery = false;
};
