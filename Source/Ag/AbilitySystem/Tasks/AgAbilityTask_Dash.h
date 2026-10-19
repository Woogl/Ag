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
	float MaxDistance = 0.f;

	bool bHasMaxDistance = false;

	virtual EAgTaskPlacement GetPlacements() const override
	{
		return EAgTaskPlacement::Window;
	}

	virtual void GetFieldRules(FAgJsonFieldRules& Rules) const override;
	virtual void Validate(const FJsonObject& Json, EAgTaskPlacement Placement, const FString& Path, TArray<FString>& Errors) override;

	virtual bool NeedsTarget() const override
	{
		return To == EAgDashTo::Target;
	}

	virtual const FAgTransition* GetTransition() const override
	{
		return Next.IsSet() ? &Next : nullptr;
	}

	virtual bool WaitsForTransition() const override
	{
		return Next.IsSet();
	}

	virtual void ValidateLinks(const FAgTaskLinks& Links, const FString& Path, TArray<FString>& Errors) const override;
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
