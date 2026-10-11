// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgBossPattern_Dash.generated.h"

class UAgAbilityTask_Dash;

/**
 * A3 돌진 찌르기 (보스 사양 '돌진 규칙', '돌진과 모션'): 돌진을 시작할 때까지 플레이어를 따라 돈 뒤, 회전 없이 플레이어를
 * 뚫고 지나가도록 곧게 돌진합니다. 돌진하는 동안이 판정 구간이고, 벽이나 가드·패리에 막히면 그 자리에서 멈춥니다.
 * 창을 내지른 자세는 돌진이 끝날 때까지 유지하고, 그 뒤 더 나아가지 않고 창을 거둡니다. 캐릭터 무브먼트는 몽타주의
 * 루트 모션이 있으면 다른 루트 모션을 쓰지 않으므로, 돌진부터는 모션 자체의 루트 모션을 무시합니다.
 */
UCLASS()
class UAgBossPattern_Dash : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgBossPattern_Dash();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	/** 돌진 상태를 켜고 끕니다: 판정 구간이 열려 회전하지 않고, 플레이어를 막지 않습니다. */
	void SetDashing(bool bNewDashing);

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

	UPROPERTY()
	TObjectPtr<UAgAbilityTask_Dash> DashTask;

	bool bDashing = false;
	bool bHolding = false;
};
