// Copyright Woogle. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "RootMotionModifier_SkewWarp.h"
#include "AbilitySystem/Tasks/AgAbilityTask_Track.h"
#include "AgRootMotionModifier_MotionWarping.generated.h"

UCLASS()
class UAgRootMotionModifier_MotionWarping : public URootMotionModifier_SkewWarp
{
	GENERATED_BODY()

public:
	UAgRootMotionModifier_MotionWarping(const FObjectInitializer& ObjectInitializer);

	void SetTrack(const FAgTrackParams& InParams, AActor* InTarget, float InGap, bool bInFacingOnly);
	virtual void Update(const FMotionWarpingUpdateContext& Context) override;
	virtual void OnStateChanged(ERootMotionModifierState LastState) override;

private:
	FAgTrackParams Params;
	TWeakObjectPtr<AActor> Target;
	FVector Origin = FVector::ZeroVector;
	float Gap = 0.f;
	bool bInitialized = false;
	bool bFacingOnly = false;
	bool bOwnsTarget = false;
};
