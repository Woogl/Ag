// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AgAbilityTask_Dash.generated.h"

class UCharacterMovementComponent;

/**
 * 루트 모션 소스로 주인을 일정한 속도로 곧게 옮깁니다(A3 돌진). 거리를 다 가거나 막히거나 Finish를 부르면 끝납니다.
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

	static UAgAbilityTask_Dash* Dash(UGameplayAbility* OwningAbility, const FVector& Direction, float Speed, float Distance);

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
	FVector StartLocation = FVector::ZeroVector;
	FVector LastLocation = FVector::ZeroVector;
	int32 BlockedTicks = 0;
	uint16 SourceID = 0;
	TWeakObjectPtr<UCharacterMovementComponent> Movement;
};
