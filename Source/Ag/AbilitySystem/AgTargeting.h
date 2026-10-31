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
}

UCLASS()
class UAgTargetingSelectionTask_Sphere : public UTargetingTask
{
	GENERATED_BODY()

public:
	UAgTargetingSelectionTask_Sphere(const FObjectInitializer& ObjectInitializer);

	virtual void Execute(const FTargetingRequestHandle& TargetingHandle) const override;
};

UCLASS()
class UAgTargetingFilterTask_Hostile : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

public:
	UAgTargetingFilterTask_Hostile(const FObjectInitializer& ObjectInitializer);

protected:
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;
};

UCLASS()
class UAgTargetingFilterTask_Hittable : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

public:
	UAgTargetingFilterTask_Hittable(const FObjectInitializer& ObjectInitializer);

protected:
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;
};

UCLASS()
class UAgTargetingFilterTask_TagRequirements : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

public:
	UAgTargetingFilterTask_TagRequirements(const FObjectInitializer& ObjectInitializer);

protected:
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;

	UPROPERTY(EditAnywhere, Category = "Target Filter")
	FGameplayTagRequirements Requirements;
};

UCLASS()
class UAgTargetingFilterTask_OnScreen : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

public:
	UAgTargetingFilterTask_OnScreen(const FObjectInitializer& ObjectInitializer);

protected:
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;
};

UCLASS()
class UAgTargetingSortTask_ScreenCenter : public UTargetingSortTask_Base
{
	GENERATED_BODY()

public:
	UAgTargetingSortTask_ScreenCenter(const FObjectInitializer& ObjectInitializer);

protected:
	virtual float GetScoreForTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;
};
