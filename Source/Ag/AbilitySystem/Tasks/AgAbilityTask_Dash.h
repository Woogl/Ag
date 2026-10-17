// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgAbilityTaskParams.h"
#include "AgAbilityTask_Dash.generated.h"

class UCharacterMovementComponent;

/**
 * 루트 모션 소스로 일정 속도로 돌진하며 선택적으로 대상을 향해 유도합니다. 거리를 다 가거나 막히거나 Finish를 부르면 끝납니다.
 * 충돌은 그대로 적용되어 벽에 막히면 그 자리에서 멈춥니다.
 */
UCLASS()
class UAgAbilityTask_Dash : public UAbilityTask
{
	GENERATED_BODY()

public:
	UAgAbilityTask_Dash();

	UPROPERTY(BlueprintAssignable)
	FGenericGameplayTaskDelegate OnFinished;

	/** HomingTurnRate는 초당 최대 수평 회전 각도입니다. 0 또는 대상 없음은 직선 돌진입니다. */
	static UAgAbilityTask_Dash* Dash(UGameplayAbility* OwningAbility, const FVector& Direction, float Speed, float Distance,
	    AActor* HomingTarget = nullptr, float HomingTurnRate = 0.f);

	static UAgAbilityTask_Dash* Dash(
	    UGameplayAbility* Owner, const FAgDashParams& Params, const FVector& Direction, const FVector& TargetLocation, AActor* Target);

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

	/** 지금 자리에서 돌진을 끝내고 OnFinished를 보냅니다. */
	void Finish();

private:
	void RemoveSource();

	FVector Direction = FVector::ForwardVector;
	float Speed = 0.f;
	float Distance = 0.f;
	float TraveledDistance = 0.f;
	float HomingTurnRate = 0.f;
	TWeakObjectPtr<AActor> HomingTarget;
	FVector LastLocation = FVector::ZeroVector;
	int32 BlockedTicks = 0;
	uint16 SourceID = 0;
	TWeakObjectPtr<UCharacterMovementComponent> Movement;
};
