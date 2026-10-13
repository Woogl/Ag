// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_Dodge.generated.h"

class UAgAbilityTask_HoldTag;

/** 회피의 전용 값 (플레이어 사양 '회피', 'SP 소모', '극한 회피', 'MP 충전', 'UP 충전', '슬로우모션') */
USTRUCT()
struct FAgDodgeParams : public FAgActionParams
{
	GENERATED_BODY()

	/** SP 소모. SP가 0보다 크면 소모량보다 적어도 쓸 수 있습니다. */
	UPROPERTY(EditAnywhere, Category = "Dodge", meta = (ClampMin = 0))
	float SPCost = 0.f;

	/** MP 충전: 극한 회피 성공 */
	UPROPERTY(EditAnywhere, Category = "Dodge", meta = (ClampMin = 0))
	float PerfectDodgeMPCharge = 0.f;

	/** UP 충전: 극한 회피 성공 */
	UPROPERTY(EditAnywhere, Category = "Dodge", meta = (ClampMin = 0))
	float PerfectDodgeUPCharge = 0.f;

	/** 회피 반격 기회: 유효 시간. 극한 회피가 발동한 순간부터 */
	UPROPERTY(EditAnywhere, Category = "Dodge", meta = (Units = "s", ClampMin = 0))
	float CounterChanceTime = 0.f;

	/** 슬로우모션: 극한 회피 성공의 속도 (게임 전체 시간 배율) */
	UPROPERTY(EditAnywhere, Category = "Dodge", meta = (ClampMin = 0.01, ClampMax = 1))
	float SlowMotionSpeed = 1.f;

	/** 슬로우모션: 극한 회피 성공의 지속 시간 (실제 시간) */
	UPROPERTY(EditAnywhere, Category = "Dodge", meta = (Units = "s", ClampMin = 0))
	float SlowMotionDuration = 0.f;
};

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

	virtual const UScriptStruct* GetActionParamsType() const override { return FAgDodgeParams::StaticStruct(); }
	virtual FAgAbilityCost GetCost() const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual bool CanPayCost(const UAgAttributeSet& Stats) const override;

private:
	/** 회피 몽타주의 노티파이(극한 회피 구간 끝, 무적 구간 끝, 후딜 시작). 이 회피의 몽타주에서 온 것만 받습니다. */
	UFUNCTION()
	void HandlePerfectDodgeWindowEnded(FGameplayEventData Payload);

	UFUNCTION()
	void HandleInvincibilityEnded(FGameplayEventData Payload);

	UFUNCTION()
	void HandleRecoveryStarted(FGameplayEventData Payload);

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
