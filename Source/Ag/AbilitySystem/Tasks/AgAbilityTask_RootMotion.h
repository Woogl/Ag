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
	TWeakObjectPtr<UAgAbilityTask_PlayMontage> Playback;
	float TargetingRange = 0.f;

	float LastPosition = 0.f;
	/** 행동 대상이 추적 거리 밖이면 락온 대상으로 바뀌며, 그때는 다가가지 않고 바라보기만 합니다(bFacingOnly). */
	TWeakObjectPtr<AActor> FacingTarget;
	FVector MoveInputDirection = FVector::ZeroVector;
	bool bFacingOnly = false;
	bool bStopped = false;

	FName WarpTargetName;
	FVector WarpGoal = FVector::ZeroVector;
	TWeakObjectPtr<URootMotionModifier_SkewWarp> Modifier;
};
