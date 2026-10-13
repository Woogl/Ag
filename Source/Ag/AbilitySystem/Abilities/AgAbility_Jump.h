// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_Jump.generated.h"

/**
 * 점프: 캐릭터 무브먼트의 점프(엔진 기본 높이와 공중 제어). 공중에서는 점프 공격 말고는 다른 행동을 시작하지 않고 이동 입력으로
 * 방향만 틉니다.
 * 달리기는 점프 중에도 이어집니다. 시작 모션은 늘 재생하고, 착지 모션은 이동 입력이 없을 때만 재생합니다.
 */
UCLASS()
class UAgAbility_Jump : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgAbility_Jump();

	/**
	 * 착지 몽타주의 ID (캐릭터 데이터의 Montages). 시작 몽타주는 어빌리티 ID로 찾습니다. 플레이어 캐릭터도 착지 모션을
	 * 멈출 때 씁니다.
	 */
	static const FName LandMontageId;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	UFUNCTION()
	void HandleLanded(EMovementMode NewMovementMode);

	/** 이동 입력이 있으면 참입니다. */
	bool IsMoving() const;
};
