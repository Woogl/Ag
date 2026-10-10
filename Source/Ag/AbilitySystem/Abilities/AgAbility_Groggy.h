// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_Groggy.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitDelay;

/**
 * 그로기 (전투 시스템 '그로기', 보스 사양 '그로기', '처형 피격'), started when PP reaches 0.
 * Stops every action and plays the groggy motion so that it ends with the groggy time. An execution replaces it with
 * the 처형 피격 motion, and groggy then ends when that motion ends, whatever time is left. PP refills when groggy ends.
 */
UCLASS()
class UAgAbility_Groggy : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgAbility_Groggy();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	UAbilityTask_PlayMontageAndWait* PlayMontage(UAnimMontage* Montage);

	/** Groggy ends: PP back to MaxPP. */
	void FinishGroggy();

	UFUNCTION()
	void HandleEndSectionDue();

	UFUNCTION()
	void HandleExecuted(FGameplayEventData Payload);

	UFUNCTION()
	void HandleMontageFinished();

	UFUNCTION()
	void HandleMontageCancelled();

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> EndSectionTask;

	bool bExecuted = false;
};
