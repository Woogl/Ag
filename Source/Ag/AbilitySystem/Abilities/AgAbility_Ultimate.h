// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AgAbility_PlayerAttack.h"
#include "AgAbility_Ultimate.generated.h"

/** 궁극기 (난무): spends the full UP gauge; dives in and strikes six times, invincible from start to end. */
UCLASS()
class UAgAbility_Ultimate : public UAgAbility_PlayerAttack
{
	GENERATED_BODY()

public:
	UAgAbility_Ultimate();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual bool CanPayCost(const UAgAttributeSet& Stats) const override;
	virtual FAgResourceAmounts GetCost() const override;

private:
	bool bInvincible = false;
};
