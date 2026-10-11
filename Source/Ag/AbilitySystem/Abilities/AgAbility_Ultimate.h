// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_Ultimate.generated.h"

/** 궁극기 (난무): UP 게이지를 모두 쓰고, 뛰어들어 여러 번 벱니다. 시작부터 끝까지 무적입니다. */
UCLASS()
class UAgAbility_Ultimate : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgAbility_Ultimate();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
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
