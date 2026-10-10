// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_PhaseTransition.generated.h"

/**
 * 페이즈 전환 (보스 사양 '연출'), started by the boss AI once the phase 2 HP zone is reached and no pattern or groggy
 * is running. Invincible while the roar plays; at its end PP refills and the phase 2 changes apply.
 */
UCLASS()
class UAgAbility_PhaseTransition : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgAbility_PhaseTransition();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	UFUNCTION()
	void HandleRoarFinished();

	UFUNCTION()
	void HandleRoarCancelled();
};
