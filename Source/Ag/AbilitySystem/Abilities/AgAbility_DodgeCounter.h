// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_DodgeCounter.generated.h"

class UAgAbilityTask_HoldTag;

/**
 * 회피 반격: 극한 회피 뒤의 회피 반격 기회에 평타 키로 씁니다. 순식간에 다가가 베며, 시작부터 판정 구간이 끝날 때까지
 * 무적입니다. 쓰면 기회가 사라집니다.
 */
UCLASS()
class UAgAbility_DodgeCounter : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgAbility_DodgeCounter();

	/** 전용 값은 공격 데이터 하나입니다. */
	virtual const UScriptStruct* GetAbilityParamsType() const override { return FAgPlayerAttack::StaticStruct(); }

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	UFUNCTION()
	void HandleLastWindowEnded();

	UFUNCTION()
	void HandleRecoveryStarted();

	UFUNCTION()
	void HandleAttackCompleted();

	UFUNCTION()
	void HandleAttackInterrupted();

	/** 시작부터 판정 구간이 끝날 때까지 무적(State.Invincible)을 붙여 둡니다. */
	UPROPERTY()
	TObjectPtr<UAgAbilityTask_HoldTag> InvincibleTag;
};
