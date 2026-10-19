// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_Dash.generated.h"

struct FGameplayEventData;

UENUM()
enum class EAgDashTo : uint8
{
	Target,
	Down
};

USTRUCT()
struct FAgDashParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	float Speed = 0.f;

	UPROPERTY()
	EAgDashTo To = EAgDashTo::Target;

	UPROPERTY()
	FAgTransition Next;

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
	static UAgAbilityTask_Dash* Dash(UGameplayAbility* Owner, const FAgDashParams& Params, int32 MontageInstanceId);

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	void TickTarget(float DeltaTime);
	void HandleAttackBlocked(const FGameplayEventData* Payload);

	void Stop();

	FAgDashParams Params;
	int32 MontageInstanceId = INDEX_NONE;

	float Distance = 0.f;
	float TraveledDistance = 0.f;
	TWeakObjectPtr<AActor> Target;
	FVector LastLocation = FVector::ZeroVector;
	int32 BlockedTicks = 0;
	FDelegateHandle BlockedHandle;
};
