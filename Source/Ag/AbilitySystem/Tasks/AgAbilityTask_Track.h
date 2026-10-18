// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgTaskParams.h"
#include "RootMotionModifier.h"
#include "AgAbilityTask_Track.generated.h"

class UAgRootMotionModifier_MotionWarping;

/**
 * 추적 구간(Track Task의 Params). 그동안 행동 대상 쪽으로 돌고, MaxWarpDistance 안에서 대상 앞 그 행동의 간격까지 다가갑니다. 거리
 * 상한은 null이면 제한 없음이고, 최대 워핑 거리 0은 위치 보정 없이 회전만 합니다.
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

	/** 반복 섹션과 겹치지 않고, 몽타주 루트 모션이 없는 구간이면 다가가지 않습니다(MaxWarpDistance 0). */
	virtual void ValidateLinks(const FAgTaskLinks& Links, const FString& Path, TArray<FString>& Errors) const override;
	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

/**
 * 추적 구간 하나. 시작할 때 대상과 간격을 정합니다. 행동 대상이 없거나 TargetingRange 밖이면 캐릭터의 대체 대상 쪽으로 돌기만 합니다.
 * 몽타주가 루트 모션을 주는 동안은 시작할 때 엔진 Motion Warping에 등록한 Modifier가 돌리고 다가가며, 몽타주 루트 모션이 꺼진
 * 동안(루트 모션 소스로 움직이는 공중 동작)은 캐릭터를 직접 돌리기만 합니다. 따라 돌다가 대상이 TargetingRange 밖으로 나가면 멈춥니다.
 */
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

	/** 구간(몽타주 시각) */
	float StartTime = 0.f;
	float EndTime = 0.f;

	/** 앞 틱의 재생 위치 */
	float LastPosition = 0.f;
	TWeakObjectPtr<AActor> Target;
	bool bFacingOnly = false;
	bool bStopped = false;
	TWeakObjectPtr<UAgRootMotionModifier_MotionWarping> Modifier;
};
