// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "Data/AgAttackData.h"
#include "AgBossPattern_Dash.generated.h"

class UAgAbilityTask_Dash;
class UAgAbilityTask_IgnoreCollision;

/** DashThrust의 전용 값 (보스 사양 '돌진 규칙') */
USTRUCT()
struct FAgBossDashParams : public FAgActionParams
{
	GENERATED_BODY()

	/** 속도 */
	UPROPERTY(EditAnywhere, Category = "Dash Thrust", meta = (Units = "cm/s", ClampMin = 0))
	float Speed = 0.f;

	/** 목표 지점: 플레이어 위치보다 이만큼 더 나아간 지점 */
	UPROPERTY(EditAnywhere, Category = "Dash Thrust", meta = (Units = "cm", ClampMin = 0))
	float Overshoot = 0.f;

	/** 최대 이동 거리 */
	UPROPERTY(EditAnywhere, Category = "Dash Thrust", meta = (Units = "cm", ClampMin = 0))
	float MaxDistance = 0.f;
};

/**
 * DashThrust (보스 사양 '돌진 규칙', '돌진과 모션'): 돌진을 시작할 때까지 플레이어를 따라 돈 뒤, 회전 없이 플레이어를
 * 뚫고 지나가도록 곧게 돌진합니다. 돌진하는 동안이 판정 구간이고, 벽이나 가드·패리에 막히면 그 자리에서 멈춥니다.
 * 창을 내지른 자세는 돌진이 끝날 때까지 유지하고, 그 뒤 더 나아가지 않고 창을 거둡니다.
 */
UCLASS()
class UAgBossPattern_Dash : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgBossPattern_Dash();

	virtual const UScriptStruct* GetActionParamsType() const override { return FAgBossDashParams::StaticStruct(); }

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	/** 돌진을 시작합니다: 판정 구간이 열려 회전하지 않고, 플레이어를 막지 않습니다. */
	UFUNCTION()
	void HandleDashStart(FGameplayEventData Payload);

	UFUNCTION()
	void HandleDashHold(FGameplayEventData Payload);

	UFUNCTION()
	void HandleDashFinished();

	UFUNCTION()
	void HandleAttackBlocked(FGameplayEventData Payload);

	UFUNCTION()
	void HandleMontageFinished();

	UFUNCTION()
	void HandleMontageCancelled();

	/** 돌진하는 동안 있습니다. */
	UPROPERTY()
	TObjectPtr<UAgAbilityTask_Dash> DashTask;

	/** 돌진하는 동안 있습니다(Pawn 채널 무시). */
	UPROPERTY()
	TObjectPtr<UAgAbilityTask_IgnoreCollision> IgnoreCollisionTask;

	/** 창을 내지른 자세로 몽타주를 멈춰 둔 동안 참입니다. */
	bool bHolding = false;
};
