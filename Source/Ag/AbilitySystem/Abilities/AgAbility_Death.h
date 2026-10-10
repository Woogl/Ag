// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_Death.generated.h"

/** 사망 (상태 우선순위 1), started by Event.Death: runs 전투 시스템 '사망 처리' on the owner. */
UCLASS()
class UAgAbility_Death : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgAbility_Death();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
