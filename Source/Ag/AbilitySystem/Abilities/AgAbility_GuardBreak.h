// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_GuardBreak.generated.h"

/**
 * 가드 브레이크 (전투 시스템 '가드 반응'): 가드가 강제로 풀리고 비틀거립니다. 하던 행동을 멈추고 공격자 쪽으로 돌아 모션을
 * 재생하며, 모션이 끝날 때까지 아무 행동도 할 수 없습니다.
 */
UCLASS()
class UAgAbility_GuardBreak : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgAbility_GuardBreak();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	UFUNCTION()
	void HandleReactionCompleted();

	UFUNCTION()
	void HandleReactionInterrupted();
};
