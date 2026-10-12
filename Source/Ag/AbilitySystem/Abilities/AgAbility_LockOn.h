// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_LockOn.generated.h"

/**
 * 락온 (플레이어 사양 '락온'): 락온하는 동안 실행됩니다. 락온 키가 시작하고, 실행 중에 다시 누르면 끝납니다.
 * 행동이 아니라서 행동 상태의 제한을 받지 않고 행동이나 피격 반응에 취소되지 않습니다(플레이어 입력 '공통 입력 처리').
 * 대상은 락온 컴포넌트가 고르고 들고 있으며, 락온 태스크가 카메라를 돌리다가 해제 조건이 되면 이 어빌리티를 끝냅니다.
 */
UCLASS()
class UAgAbility_LockOn : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgAbility_LockOn();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	/** 플레이어가 락온 키를 다시 눌렀습니다(이 어빌리티가 실행 중일 때 플레이어 캐릭터가 보냄). */
	UFUNCTION()
	void HandleLockOnInput(FGameplayEventData Payload);

	UFUNCTION()
	void HandleTargetLost();
};
