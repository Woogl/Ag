// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionJumpForce.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_Launch.generated.h"

UENUM()
enum class EAgLaunchTo : uint8
{
	Target
};

USTRUCT()
struct FAgLaunchParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	float AirTime = 0.f;

	UPROPERTY()
	float Height = 0.f;

	UPROPERTY()
	EAgLaunchTo To = EAgLaunchTo::Target;

	UPROPERTY()
	float MaxDistance = 0.f;

	bool bHasHeight = false;
	bool bHasTo = false;
	bool bHasMaxDistance = false;

	virtual EAgTaskPlacement GetPlacements() const override
	{
		return EAgTaskPlacement::Activation | EAgTaskPlacement::Point;
	}

	virtual void GetFieldRules(FAgJsonFieldRules& Rules) const override;
	virtual void Validate(const FJsonObject& Json, EAgTaskPlacement Placement, const FString& Path, TArray<FString>& Errors) override;

	virtual bool NeedsTarget() const override
	{
		return bHasTo;
	}

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

UCLASS()
class UAgAbilityTask_Launch : public UAbilityTask_ApplyRootMotionJumpForce
{
	GENERATED_BODY()

public:
	static const FName SourceName;

	static UAgAbilityTask_Launch* Launch(UGameplayAbility* Owner, const FAgLaunchParams& Params);

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;

private:
	FAgLaunchParams Params;
	bool bReportedRootMotion = false;
};
