// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_ScaleRootMotion.generated.h"

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

	virtual void ValidateLinks(const FAgTaskLinks& Links, const FString& Path, TArray<FString>& Errors) const override;
	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

/** 발동 Task는 몽타주를 재생하기 전(같은 프레임)에 시작하므로 첫 루트 모션부터 적용됩니다. */
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

	float Scale = 0.f;
};
