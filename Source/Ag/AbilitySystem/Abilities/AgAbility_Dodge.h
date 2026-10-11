// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_Dodge.generated.h"

class UAgAbilityTask_HoldTag;

/**
 * 회피: rolls toward the movement input (camera-relative), or steps back without input.
 * Invincible and in the 극한 회피 구간 from the start for the data's times, measured on the motion.
 * A boss attack touching the 극한 회피 구간 triggers 극한 회피 once per dodge: the SP spent comes back, MP and UP charge,
 * the 회피 반격 기회 opens and the slow motion starts.
 */
UCLASS()
class UAgAbility_Dodge : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgAbility_Dodge();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual float GetCooldownDuration() const override;
	virtual bool CanPayCost(const UAgAttributeSet& Stats) const override;
	virtual FAgAbilityCost GetCost() const override;

private:
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

	/** 회피 무적 구간과 극한 회피 구간: 회피 시작부터 데이터의 시간까지 태그를 붙여 둡니다. */
	UPROPERTY()
	TObjectPtr<UAgAbilityTask_HoldTag> InvincibleTag;

	UPROPERTY()
	TObjectPtr<UAgAbilityTask_HoldTag> PerfectDodgeWindowTag;

	/** SP actually spent by this dodge (less than the cost when SP was low), returned on 극한 회피. */
	float SPSpent = 0.f;

	bool bPerfectDodged = false;
};
