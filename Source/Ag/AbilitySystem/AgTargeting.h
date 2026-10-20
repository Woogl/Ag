// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Tasks/TargetingFilterTask_BasicFilterTemplate.h"
#include "Tasks/TargetingTask.h"
#include "AgTargeting.generated.h"

struct FAgTargetingRequest;
struct FTargetingRequestHandle;

struct FAgTargetingConditions
{
	float Range = 0.f;

	FGameplayTagRequirements TagRequirements;

	static FAgTargetingConditions& FindOrAdd(FTargetingRequestHandle Handle);
	static FAgTargetingConditions* Find(FTargetingRequestHandle Handle);
};

namespace AgTargeting
{
	TArray<AActor*> FindTargets(const FAgTargetingRequest& Request, AActor& Source);
	AActor* FindTarget(const FAgTargetingRequest& Request, AActor& Source);
}

UCLASS(meta = (DisplayName = "Ag Select In Range"))
class UAgTargetingSelectionTask_Range : public UTargetingTask
{
	GENERATED_BODY()

public:
	UAgTargetingSelectionTask_Range(const FObjectInitializer& ObjectInitializer);

	virtual void Execute(const FTargetingRequestHandle& TargetingHandle) const override;
};

UCLASS(meta = (DisplayName = "Ag Filter Hostile"))
class UAgTargetingFilterTask_Hostile : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

public:
	UAgTargetingFilterTask_Hostile(const FObjectInitializer& ObjectInitializer);

protected:
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;
};

UCLASS(meta = (DisplayName = "Ag Filter Tag Requirements"))
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

UCLASS(meta = (DisplayName = "Ag Filter On Screen"))
class UAgTargetingFilterTask_OnScreen : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

public:
	UAgTargetingFilterTask_OnScreen(const FObjectInitializer& ObjectInitializer);

protected:
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;
};
