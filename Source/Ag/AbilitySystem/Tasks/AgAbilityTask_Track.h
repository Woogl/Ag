// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgTaskParams.h"
#include "RootMotionModifier.h"
#include "AgAbilityTask_Track.generated.h"

class UAgRootMotionModifier_MotionWarping;

USTRUCT()
struct FAgTrackParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	float TargetingRange = 0.f;

	UPROPERTY()
	float MaxWarpDistance = 0.f;

	UPROPERTY()
	EMotionWarpRotationMethod RotationMethod = EMotionWarpRotationMethod::ConstantRate;

	bool bHasTargetingRange = false;
	bool bHasMaxWarpDistance = false;

	virtual EAgTaskPlacement GetPlacements() const override
	{
		return EAgTaskPlacement::Window;
	}

	virtual void GetFieldRules(FAgJsonFieldRules& Rules) const override;
	virtual void Validate(const FJsonObject& Json, EAgTaskPlacement Placement, const FString& Path, TArray<FString>& Errors) override;

	virtual void ValidateLinks(const FAgTaskLinks& Links, const FString& Path, TArray<FString>& Errors) const override;
	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

UCLASS()
class UAgAbilityTask_Track : public UAbilityTask
{
	GENERATED_BODY()

public:
	UAgAbilityTask_Track();

	static UAgAbilityTask_Track* Track(UGameplayAbility* Owner, const FAgTrackParams& Params, int32 MontageInstanceId, float StartTime, float EndTime);

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	FAgTrackParams Params;
	int32 MontageInstanceId = INDEX_NONE;

	float StartTime = 0.f;
	float EndTime = 0.f;

	float LastPosition = 0.f;
	TWeakObjectPtr<AActor> Target;
	bool bFacingOnly = false;
	bool bStopped = false;
	TWeakObjectPtr<UAgRootMotionModifier_MotionWarping> Modifier;
};
