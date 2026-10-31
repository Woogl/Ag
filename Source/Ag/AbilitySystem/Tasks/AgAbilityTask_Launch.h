// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_Launch.generated.h"

struct FGameplayEventData;

UENUM()
enum class EAgLaunchDirection : uint8
{
	Up,
	Back,
	Target
};

USTRUCT()
struct FAgLaunchParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	TOptional<float> AirTime;

	UPROPERTY()
	TOptional<float> Height;

	UPROPERTY()
	TOptional<float> Altitude;

	UPROPERTY()
	EAgLaunchDirection Direction = EAgLaunchDirection::Up;

	UPROPERTY()
	float Distance = 0.f;

	UPROPERTY()
	TOptional<float> MaxDistance;

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

UCLASS()
class UAgAbilityTask_Launch : public UAbilityTask
{
	GENERATED_BODY()
	friend struct FAgLaunchParams;

public:
	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	void HandleLanded(const FGameplayEventData* Payload);

	UPROPERTY()
	FAgLaunchParams Params;

	FDelegateHandle LandedHandle;
	bool bMontageRootMotionDisabled = false;
};
