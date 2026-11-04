// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Tasks/TargetingFilterTask_BasicFilterTemplate.h"
#include "Tasks/TargetingSortTask_Base.h"
#include "Tasks/TargetingTask.h"
#include "AgTargeting.generated.h"

struct FAgTargetingRequest;

namespace AgTargeting
{
	TArray<AActor*> FindTargets(const FAgTargetingRequest& Request, AActor& Source);
	AActor* FindTarget(const FAgTargetingRequest& Request, AActor& Source);
	bool IsInRange(const AActor& Source, const AActor& Target, float Range);

	/** 출처가 AI면 지각의 시야가 대상을 지금 감지하고, Socket이 있으면 출처 메시의 그 소켓에서 대상 캡슐 중심까지 WorldStatic이 가리지 않습니다. */
	bool IsVisible(const AActor& Source, const AActor& Target, FName Socket);

	/** Source가 Target 정면의 등 쪽 90°(바로 뒤에서 좌우 45°) 안에 있습니다. */
	bool IsBehind(const AActor& Target, const AActor& Source);
}

UCLASS()
class UAgTargetingSelectionTask_Sphere : public UTargetingTask
{
	GENERATED_BODY()

public:
	virtual void Execute(const FTargetingRequestHandle& TargetingHandle) const override;
};

UCLASS()
class UAgTargetingFilterTask_Hostile : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

protected:
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;
};

UCLASS()
class UAgTargetingFilterTask_Hittable : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

protected:
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;
};

UCLASS()
class UAgTargetingFilterTask_TagRequirements : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

protected:
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;

	UPROPERTY(EditAnywhere, Category = "Target Filter")
	FGameplayTagRequirements Requirements;
};

UCLASS()
class UAgTargetingFilterTask_OnScreen : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

protected:
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;
};

UCLASS()
class UAgTargetingFilterTask_Visible : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

protected:
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;

	UPROPERTY(EditAnywhere, Category = "Target Filter")
	FName Socket;
};

UCLASS()
class UAgTargetingSortTask_ScreenCenter : public UTargetingSortTask_Base
{
	GENERATED_BODY()

protected:
	virtual float GetScoreForTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;
};
