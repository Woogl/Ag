// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_Dodge.generated.h"

class UAgAbilityTask_HoldTag;

/**
 * 회피: 이동 입력 쪽(카메라 기준)으로 구르고, 입력이 없으면 뒤로 물러납니다. 시작부터 데이터의 시간 동안 무적이고 극한 회피
 * 구간이며, 시간은 몽타주 재생 위치로 잽니다. 극한 회피 구간에 공격이 닿으면 회피 한 번에 한 번 극한 회피가 발동해, 쓴 SP를
 * 돌려받고 MP·UP를 충전하며 회피 반격 기회가 생기고 슬로우모션이 시작됩니다.
 */
UCLASS()
class UAgAbility_Dodge : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgAbility_Dodge();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual float GetCooldownDuration() const override;
	virtual bool CanPayCost(const UAgAttributeSet& Stats) const override;
	virtual FAgAbilityCost GetCost() const override;

private:
	UFUNCTION()
	void HandlePerfectDodgeWindowEnded();

	UFUNCTION()
	void HandleInvincibilityEnded();

	UFUNCTION()
	void HandleRecoveryStarted();

	UFUNCTION()
	void HandlePerfectDodge(FGameplayEventData Payload);

	UFUNCTION()
	void HandleMontageFinished();

	UFUNCTION()
	void HandleMontageCancelled();

	/** 회피 무적 구간과 극한 회피 구간: 회피 시작부터 데이터의 시간까지 태그를 붙여 둡니다. */
	UPROPERTY()
	TObjectPtr<UAgAbilityTask_HoldTag> InvincibleTag;

	UPROPERTY()
	TObjectPtr<UAgAbilityTask_HoldTag> PerfectDodgeWindowTag;

	/** 이번 회피에 실제로 쓴 SP(SP가 모자랐으면 비용보다 적음). 극한 회피 때 돌려받습니다. */
	float SPSpent = 0.f;

	bool bPerfectDodged = false;
};
