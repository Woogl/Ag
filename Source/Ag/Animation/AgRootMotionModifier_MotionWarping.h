// Copyright Woogle. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "RootMotionModifier_SkewWarp.h"
#include "Data/AgTaskParams.h"
#include "AgRootMotionModifier_MotionWarping.generated.h"
class UAgGameplayAbility;

/**
 * 추적 구간(Track)의 값. Task가 아니라 Runner가 맡는 시간표 구간입니다. 몽타주가 루트 모션을 주면 엔진 Motion Warping에 등록하고,
 * 주지 않으면 Runner가 캐릭터를 직접 돌립니다. 거리 상한은 null이면 제한 없음이고, 최대 워핑 거리 0은 위치 보정 없이 회전만
 * 합니다.
 */
USTRUCT()
struct FAgTrackParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	float TargetingRange = 0.f;

	UPROPERTY()
	float MaxWarpDistance = 0.f;

	/** ConstantRate나 Slerp */
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
};

/** 몽타주 루트 모션이 있는 추적 구간 하나. 구간의 값과 재생을 소유한 Runner의 행동 대상과 간격만 씁니다. */
UCLASS()
class UAgRootMotionModifier_MotionWarping : public URootMotionModifier_SkewWarp
{
	GENERATED_BODY()

public:
	UAgRootMotionModifier_MotionWarping(const FObjectInitializer& ObjectInitializer);

	/** 추적 구간의 값 */
	void SetTrackSettings(const FAgTrackParams& InParams);
	virtual void Update(const FMotionWarpingUpdateContext& Context) override;
	virtual void OnStateChanged(ERootMotionModifierState LastState) override;

private:
	FAgTrackParams Params;
	TWeakObjectPtr<UAgGameplayAbility> OwnerAbility;
	TWeakObjectPtr<AActor> Target;
	FVector Origin = FVector::ZeroVector;
	float Gap = 0.f;
	int32 MontageInstanceId = INDEX_NONE;
	bool bInitialized = false;
	bool bFacingOnly = false;
	bool bOwnsTarget = false;
};
