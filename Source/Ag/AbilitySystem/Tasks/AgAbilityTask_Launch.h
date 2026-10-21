// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionJumpForce.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_Launch.generated.h"

UENUM()
enum class EAgLaunchTo : uint8
{
	Up,
	Target
};

USTRUCT()
struct FAgLaunchParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	float AirTime = 0.f;

	UPROPERTY()
	TOptional<float> Height;

	UPROPERTY()
	EAgLaunchTo To = EAgLaunchTo::Up;

	UPROPERTY()
	TOptional<float> MaxDistance;

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

UCLASS()
class UAgAbilityTask_Launch : public UAbilityTask_ApplyRootMotionJumpForce
{
	GENERATED_BODY()
	friend struct FAgLaunchParams;

public:
	static const FName SourceName;

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;

private:
	UPROPERTY()
	FAgLaunchParams Params;

	bool bReportedRootMotion = false;
};
