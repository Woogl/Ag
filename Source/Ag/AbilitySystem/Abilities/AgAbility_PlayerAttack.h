// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_PlayerAttack.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;
struct FAgPlayerAttack;

/**
 * A player attack action (평타 한 타, 스킬, 궁극기): plays its montages one after another with its attack data,
 * sets the attack direction and movement for every hit (플레이어 사양 '공격 방향', '공격 중 이동')
 * and starts the recovery (후딜) when the last attack window ends.
 * Subclasses commit through ActivateAbility and then call StartAttack.
 */
UCLASS(Abstract)
class UAgAbility_PlayerAttack : public UAgGameplayAbility
{
	GENERATED_BODY()

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** Starts (or restarts) the attack. Montages play back to back, each blended into the next. Ends the ability if there is nothing to play. */
	void StartAttack(const FAgPlayerAttack& Attack, TConstArrayView<TObjectPtr<UAnimMontage>> InMontages);

	bool IsInRecovery() const { return bInRecovery; }

private:
	void PlayMontageAt(int32 Index);

	UFUNCTION()
	void HandleAttackWindowEnded(FGameplayEventData Payload);

	UFUNCTION()
	void HandleMontageFinished();

	UFUNCTION()
	void HandleMontageCancelled();

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	UPROPERTY()
	TArray<TObjectPtr<UAnimMontage>> Montages;

	int32 MontageIndex = 0;
	int32 HitCount = 0;
	float TargetingRange = 0.f;
	bool bInRecovery = false;
};
