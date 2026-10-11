// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_KnockUp.generated.h"

class UAgAbilityTask_PlayReaction;
class UAnimMontage;

/**
 * 넉업: 공중으로 떠올랐다가 떨어진 뒤 일어남 (전투 시스템 '넉업').
 * 루트 모션 점프로 중력과 관계없이 높이와 체공 시간의 포물선을 따라 곧게 오르내립니다. 그동안 모션의 루트 이동은 꺼서
 * 떨어진 자리에서 착지합니다. 정점 무렵 상승 자세가 낙하 자세로 바뀌고, 착지하면 낙하 몽타주의 Land 섹션과 기상을 잇습니다.
 * 하던 행동을 멈추고 공격자 쪽으로 돌아 시작하며, 바닥에 닿은 순간부터 기상이 끝날 때까지 무적입니다.
 */
UCLASS()
class UAgAbility_KnockUp : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgAbility_KnockUp();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	UFUNCTION()
	void HandleFalling();

	UFUNCTION()
	void HandleLanded(EMovementMode NewMovementMode);

	UFUNCTION()
	void HandleReactionCompleted();

	UFUNCTION()
	void HandleReactionInterrupted();

	UAnimMontage* GetFallMontage() const;

	UPROPERTY()
	TObjectPtr<UAgAbilityTask_PlayReaction> ReactionTask;

	bool bFalling = false;
};
