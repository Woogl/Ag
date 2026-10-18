// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_ScaleRootMotion.generated.h"

/** 루트 이동 거리(ScaleRootMotion Task의 Params). Distance는 행동 몽타주 전체 루트 모션의 수평 이동 거리(cm)입니다. */
USTRUCT()
struct FAgScaleRootMotionParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	float Distance = 0.f;

	virtual EAgTaskPlacement GetPlacements() const override
	{
		return EAgTaskPlacement::Activation;
	}

	virtual void Validate(const FJsonObject& Json, EAgTaskPlacement Placement, const FString& Path, TArray<FString>& Errors) override;

	/** 몽타주 전체 루트 이동에 대한 배율로 맞추므로, 그 행동의 몽타주가 수평으로 움직여야 합니다. */
	virtual void ValidateLinks(const FAgTaskLinks& Links, const FString& Path, TArray<FString>& Errors) const override;
	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

/**
 * 행동 몽타주 전체 루트 모션의 수평 이동 거리가 Distance가 되도록 캐릭터의 루트 모션 이동 배율을 정하고, 끝날 때 건 배율이 그대로면
 * 되돌립니다. 발동 Task는 몽타주를 재생한 같은 프레임에 시작하므로 첫 루트 모션부터 적용됩니다.
 */
UCLASS()
class UAgAbilityTask_ScaleRootMotion : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UAgAbilityTask_ScaleRootMotion* ScaleRootMotion(UGameplayAbility* Owner, float Distance);

	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	float Distance = 0.f;

	/** 건 배율. 걸지 않았으면 0입니다. */
	float Scale = 0.f;
};
