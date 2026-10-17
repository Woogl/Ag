// Copyright Woogle. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "RootMotionModifier_SkewWarp.h"
#include "Data/AgAbilityTaskParams.h"
#include "AgRootMotionModifier_MotionWarping.generated.h"
class UAgGameplayAbility;

/** 몽타주 시간표의 워핑 항목 하나. 항목의 값과 재생을 소유한 어빌리티가 정한 대상만 씁니다. 공격 판정을 조회하지 않습니다. */
UCLASS()
class UAgRootMotionModifier_MotionWarping : public URootMotionModifier_SkewWarp
{
	GENERATED_BODY()

public:
	UAgRootMotionModifier_MotionWarping(const FObjectInitializer& ObjectInitializer);

	/** 워핑 항목의 값. 최대 워핑 거리 0은 위치 보정 없이 회전만 합니다. */
	void SetTimelineSettings(const FAgMotionWarpingParams& InParams, EMotionWarpRotationMethod InRotationMethod);
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
