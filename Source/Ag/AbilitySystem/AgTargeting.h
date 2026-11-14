// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Tasks/SimpleTargetingSelectionTask.h"
#include "Tasks/TargetingFilterTask_BasicFilterTemplate.h"
#include "Tasks/TargetingSortTask_Base.h"
#include "AgTargeting.generated.h"

struct FAgTargetingRequest;

namespace AgTargeting
{
	TArray<AActor*> FindTargets(const FAgTargetingRequest& Request, AActor& Source);
	AActor* FindTarget(const FAgTargetingRequest& Request, AActor& Source);
	bool IsInRange(const AActor& Source, const AActor& Target, float Range);

	/** 후보 가운데 게임 카메라 화면에서 Current보다 Direction 쪽(양수 = 오른쪽)에 있고 Current와 가로로 가장 가까운 대상입니다. */
	AActor* FindSideTarget(const FAgTargetingRequest& Request, APawn& Source, const AActor& Current, float Direction);
}

UCLASS()
class UAgTargetingSelectionTask_Sphere : public USimpleTargetingSelectionTask
{
	GENERATED_BODY()

public:
	virtual void SelectTargets_Implementation(const FTargetingRequestHandle& TargetingHandle, const FTargetingSourceContext& SourceContext) const override;
};

/** 출처의 락온 대상을 거리와 관계없이 후보에 더합니다. */
UCLASS()
class UAgTargetingSelectionTask_LockOnTarget : public USimpleTargetingSelectionTask
{
	GENERATED_BODY()

public:
	virtual void SelectTargets_Implementation(const FTargetingRequestHandle& TargetingHandle, const FTargetingSourceContext& SourceContext) const override;
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

/** 출처에게 락온 대상이 있으면 그 대상만 남깁니다. 락온 중이 아니면 거르지 않습니다. */
UCLASS()
class UAgTargetingFilterTask_LockOnTarget : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

protected:
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;
};

UCLASS()
class UAgTargetingFilterTask_OnScreen : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

protected:
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;
};

UCLASS()
class UAgTargetingFilterTask_LineOfSight : public UTargetingFilterTask_BasicFilterTemplate
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
