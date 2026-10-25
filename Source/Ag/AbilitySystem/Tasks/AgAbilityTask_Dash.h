// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_Dash.generated.h"

struct FGameplayEventData;

USTRUCT()
struct FAgDashParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	float Speed = 0.f;

	UPROPERTY()
	float HomingTurnRate = 0.f;

	UPROPERTY()
	float Overshoot = 0.f;

	UPROPERTY()
	TOptional<float> MaxDistance;

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

UCLASS()
class UAgAbilityTask_Dash : public UAbilityTask_ApplyRootMotionConstantForce
{
	GENERATED_BODY()
	friend struct FAgDashParams;

public:
	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	void HandleAttackBlocked(const FGameplayEventData* Payload);
	void HandleMoveBlocked(const FGameplayEventData* Payload);

	UPROPERTY()
	FAgDashParams Params;

	bool bDashing = false;

	TWeakObjectPtr<AActor> Target;
	FDelegateHandle AttackBlockedHandle;
	FDelegateHandle MoveBlockedHandle;
};
