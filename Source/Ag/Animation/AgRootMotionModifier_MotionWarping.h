// Copyright Woogle. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "RootMotionModifier_SkewWarp.h"
#include "AbilitySystem/Tasks/AgAbilityTask_Track.h"
#include "AgRootMotionModifier_MotionWarping.generated.h"

/** 몽타주 루트 모션이 있는 추적 구간 하나. 추적 Task가 구간을 시작할 때 만들어 등록하고, 그 Task가 정한 대상과 간격으로 워핑합니다. */
UCLASS()
class UAgRootMotionModifier_MotionWarping : public URootMotionModifier_SkewWarp
{
	GENERATED_BODY()

public:
	UAgRootMotionModifier_MotionWarping(const FObjectInitializer& ObjectInitializer);

	/** 추적 구간의 값과 대상·간격. bInFacingOnly면 다가가지 않고 돌기만 합니다. */
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
