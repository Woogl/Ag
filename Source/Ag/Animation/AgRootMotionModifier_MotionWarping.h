// Copyright Woogle. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "RootMotionModifier_SkewWarp.h"
#include "Data/AgAbilityTaskParams.h"
#include "AgRootMotionModifier_MotionWarping.generated.h"
class UAgGameplayAbility;

/** 엔진 구간에서 고정 Params와 지정된 실행 목표만 사용합니다. 공격 판정을 조회하지 않습니다. */
UCLASS(meta = (DisplayName = "Ag Motion Warping"))
class UAgRootMotionModifier_MotionWarping : public URootMotionModifier_SkewWarp
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Motion Warping")
	FName ParamsId;
	virtual void Update(const FMotionWarpingUpdateContext& Context) override;
	virtual void OnStateChanged(ERootMotionModifierState LastState) override;

private:
	FAgMotionWarpingParams Params;
	TWeakObjectPtr<UAgGameplayAbility> OwnerAbility;
	TWeakObjectPtr<AActor> Target;
	FVector Origin = FVector::ZeroVector;
	float Gap = 0.f;
	int32 MontageInstanceId = INDEX_NONE;
	bool bInitialized = false;
	bool bFacingOnly = false;
	bool bOwnsTarget = false;
};
