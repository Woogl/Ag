// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_GuardPushback.generated.h"

/**
 * 가드 밀림 (전투 시스템 '가드 반응'): 가드를 유지한 채 밀려나며, 그동안 받은 공격도 가드합니다. 하던 행동을 멈추고 공격자 쪽으로
 * 돌아 모션을 재생하며, 모션이 끝날 때까지 아무 행동도 할 수 없습니다.
 */
UCLASS()
class UAgAbility_GuardPushback : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgAbility_GuardPushback();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	UFUNCTION()
	void HandleReactionCompleted();

	UFUNCTION()
	void HandleReactionInterrupted();
};
