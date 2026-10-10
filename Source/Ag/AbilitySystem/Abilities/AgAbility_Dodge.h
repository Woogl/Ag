// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_Dodge.generated.h"

/**
 * 회피: rolls toward the movement input (camera-relative), or steps back without input.
 * Invincible and in the 극한 회피 구간 from the start for the data's times, measured on the motion.
 * A boss attack touching the 극한 회피 구간 triggers 극한 회피 once per dodge: the SP spent comes back and MP and UP charge.
 */
UCLASS()
class UAgAbility_Dodge : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgAbility_Dodge();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual float GetCooldownDuration() const override;
	virtual bool CanPayCost(const UAgAttributeSet& Stats) const override;
	virtual FAgResourceAmounts GetCost() const override;

private:
	/** Adds or removes a loose state tag this ability owns for part of the dodge. */
	void SetStateTag(const FGameplayTag& Tag, bool& bHasTag, bool bWanted);

	UFUNCTION()
	void HandlePerfectDodgeWindowEnded();

	UFUNCTION()
	void HandleInvincibilityEnded();

	UFUNCTION()
	void HandleRecoveryStarted();

	UFUNCTION()
	void HandlePerfectDodge(FGameplayEventData Payload);

	UFUNCTION()
	void HandleMontageFinished();

	UFUNCTION()
	void HandleMontageCancelled();

	/** SP actually spent by this dodge (less than the cost when SP was low), returned on 극한 회피. */
	float SPSpent = 0.f;

	bool bPerfectDodged = false;
	bool bInPerfectDodgeWindow = false;
	bool bInvincible = false;
};
