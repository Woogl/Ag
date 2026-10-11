// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_KnockBack.generated.h"

/**
 * 넉백: 뒤로 크게 밀려나며 쓰러졌다가 일어남 (전투 시스템 '피격 반응'). 플레이어와 보스 모두 씁니다.
 * 넉백 모션 하나에 쓰러짐과 기상이 다 들어 있고, 모션의 루트 이동대로 물러납니다. 하던 행동이나 보스 패턴을 멈추고 공격자 쪽으로
 * 돌아 모션을 재생하며, 모션이 끝날 때까지 아무 행동도 할 수 없습니다. 바닥에 쓰러진 순간부터 모션이 끝날 때까지 무적입니다.
 */
UCLASS()
class UAgAbility_KnockBack : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgAbility_KnockBack();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	UFUNCTION()
	void HandleReactionCompleted();

	UFUNCTION()
	void HandleReactionInterrupted();
};
