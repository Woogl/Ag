// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_Jump.generated.h"

/**
 * 점프: 캐릭터 무브먼트의 점프(엔진 기본 높이와 공중 제어). 공중에서는 다른 행동을 시작하지 않고 이동 입력으로 방향만 틉니다.
 * 달리기는 점프 중에도 이어집니다. 시작 모션은 늘 재생하고, 착지 모션은 이동 입력이 없을 때만 재생합니다.
 */
UCLASS()
class UAgAbility_Jump : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgAbility_Jump();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	UFUNCTION()
	void HandleLanded(EMovementMode NewMovementMode);

	/** True while there is movement input. */
	bool IsMoving() const;
};
