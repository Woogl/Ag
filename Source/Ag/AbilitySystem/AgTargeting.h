// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Tasks/TargetingFilterTask_BasicFilterTemplate.h"
#include "AgTargeting.generated.h"

struct FAgAbilityTargeting;
struct FTargetingRequestHandle;

struct FAgTargetingConditions
{
	float Range = -1.f;

	FGameplayTagRequirements TagRequirements;

	static FAgTargetingConditions& FindOrAdd(FTargetingRequestHandle Handle);
	static FAgTargetingConditions* Find(FTargetingRequestHandle Handle);
};

namespace AgTargeting
{
	AActor* FindTarget(const FAgAbilityTargeting& Targeting, AActor& Source);
}

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

UCLASS(meta = (DisplayName = "Ag Filter Distance"))
class UAgTargetingFilterTask_Distance : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

public:
	UAgTargetingFilterTask_Distance(const FObjectInitializer& ObjectInitializer);

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
