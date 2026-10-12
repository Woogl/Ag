// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_Death.generated.h"

/**
 * 사망 (상태 우선순위 1): Event.Death로 시작해 전투 시스템 '사망 처리'를 합니다.
 * 처형으로 죽은 보스는 처형 피격 모션에서 이어 처형 사망 모션을 끝까지 재생한 뒤 래그돌이 됩니다.
 */
UCLASS()
class UAgAbility_Death : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgAbility_Death();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	UFUNCTION()
	void HandleExecutedDeathEnded();
};
