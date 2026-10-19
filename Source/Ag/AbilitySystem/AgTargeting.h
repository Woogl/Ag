// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Tasks/TargetingFilterTask_BasicFilterTemplate.h"
#include "Tasks/TargetingSortTask_Base.h"
#include "Tasks/TargetingTask.h"
#include "AgTargeting.generated.h"

struct FAgAbilityTargeting;
struct FTargetingRequestHandle;

struct FAgTargetingConditions
{
	float Range = -1.f;

	FGameplayTagContainer RequiredTags;

	FGameplayTagContainer BlockedTags;

	static FAgTargetingConditions& FindOrAdd(FTargetingRequestHandle Handle);
	static FAgTargetingConditions* Find(FTargetingRequestHandle Handle);
};

namespace AgTargeting
{
	AActor* FindTarget(const FAgAbilityTargeting& Targeting, AActor& Source);
}

UCLASS(meta = (DisplayName = "Ag Filter Action Target"))
class UAgTargetingFilterTask_ActionTarget : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

public:
	UAgTargetingFilterTask_ActionTarget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;
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

/** 거르는 Task가 후보의 순서를 섞으므로 프리셋에서 거르는 Task 뒤에 둡니다. */
UCLASS(meta = (DisplayName = "Ag Sort Horizontal Distance"))
class UAgTargetingSortTask_HorizontalDistance : public UTargetingSortTask_Base
{
	GENERATED_BODY()

public:
	UAgTargetingSortTask_HorizontalDistance(const FObjectInitializer& ObjectInitializer);

protected:
	virtual float GetScoreForTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;
};

UCLASS(meta = (DisplayName = "Ag Select AI Target"))
class UAgTargetingSelectionTask_AITarget : public UTargetingTask
{
	GENERATED_BODY()

public:
	UAgTargetingSelectionTask_AITarget(const FObjectInitializer& ObjectInitializer);

	virtual void Execute(const FTargetingRequestHandle& TargetingHandle) const override;
};
