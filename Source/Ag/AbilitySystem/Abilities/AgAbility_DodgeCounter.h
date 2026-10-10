// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AgAbility_PlayerAttack.h"
#include "AgAbility_DodgeCounter.generated.h"

/**
 * 회피 반격: the attack key during the 회피 반격 기회 after a 극한 회피. Closes in fast and slashes, invincible from the
 * start until its attack window ends. Using it ends the chance.
 */
UCLASS()
class UAgAbility_DodgeCounter : public UAgAbility_PlayerAttack
{
	GENERATED_BODY()

public:
	UAgAbility_DodgeCounter();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	UFUNCTION()
	void HandleInvincibleWindowEnded(FGameplayEventData Payload);

	void SetInvincible(bool bNewInvincible);

	bool bInvincible = false;
};
