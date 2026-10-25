// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgTaskParams.h"
#include "GameplayTagContainer.h"
#include "RootMotionModifier.h"
#include "AgAbilityTask_RootMotion.generated.h"

class URootMotionModifier_SkewWarp;

UENUM()
enum class EAgRootMotionDirection : uint8
{
	Target,
	Back,
	MoveInput
};

USTRUCT()
struct FAgRootMotionParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	EAgRootMotionDirection Direction = EAgRootMotionDirection::Target;

	UPROPERTY()
	bool bWarpTranslation = true;

	UPROPERTY()
	TOptional<float> TargetingRange;

	UPROPERTY()
	EMotionWarpRotationMethod RotationMethod = EMotionWarpRotationMethod::ConstantRate;

	UPROPERTY()
	float Distance = 0.f;

	UPROPERTY()
	FGameplayTagContainer RequiredTags;

	UPROPERTY()
	FGameplayTagContainer BlockedTags;

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

UCLASS()
class UAgAbilityTask_RootMotion : public UAbilityTask
{
	GENERATED_BODY()
	friend struct FAgRootMotionParams;

public:
	UAgAbilityTask_RootMotion();

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	bool HasGoal() const;
	FVector GetFacingDirection() const;
	void RemoveWarpTarget();

	UPROPERTY()
	FAgRootMotionParams Params;

	TOptional<FFloatInterval> Window;
	float TargetingRange = 0.f;

	float LastPosition = 0.f;
	TWeakObjectPtr<AActor> Target;
	FVector MoveInputDirection = FVector::ZeroVector;
	bool bFacingOnly = false;
	bool bStopped = false;

	FName WarpTargetName;
	FVector WarpGoal = FVector::ZeroVector;
	TWeakObjectPtr<URootMotionModifier_SkewWarp> Modifier;
};
