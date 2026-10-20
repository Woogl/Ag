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

public:
	static UAgAbilityTask_Dash* Dash(UGameplayAbility* Owner, const FAgDashParams& Params);

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	void HandleBlocked(const FGameplayEventData* Payload);

	FAgDashParams Params;
	bool bTagAdded = false;
	int32 MontageInstanceId = INDEX_NONE;

	TWeakObjectPtr<AActor> Target;
	FDelegateHandle AttackBlockedHandle;
	FDelegateHandle MoveBlockedHandle;
};
