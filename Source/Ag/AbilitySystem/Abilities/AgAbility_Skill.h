// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AgAbility_PlayerAttack.h"
#include "AgAbility_Skill.generated.h"

/** 스킬 (돌진 베기): an MP-costing dash slash with a cooldown. */
UCLASS()
class UAgAbility_Skill : public UAgAbility_PlayerAttack
{
	GENERATED_BODY()

public:
	UAgAbility_Skill();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual float GetCooldownDuration() const override;
	virtual bool CanPayCost(const UAgAttributeSet& Stats) const override;
	virtual FAgResourceAmounts GetCost() const override;
};
