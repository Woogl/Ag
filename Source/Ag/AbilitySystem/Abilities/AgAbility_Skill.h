// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_Skill.generated.h"

/** 스킬 (돌진 베기): MP를 쓰고 쿨다운이 있는 돌진 베기입니다. */
UCLASS()
class UAgAbility_Skill : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgAbility_Skill();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual float GetCooldownDuration() const override;
	virtual bool CanPayCost(const UAgAttributeSet& Stats) const override;
	virtual FAgAbilityCost GetCost() const override;

private:
	UFUNCTION()
	void HandleLastWindowEnded();

	UFUNCTION()
	void HandleAttackCompleted();

	UFUNCTION()
	void HandleAttackInterrupted();
};
