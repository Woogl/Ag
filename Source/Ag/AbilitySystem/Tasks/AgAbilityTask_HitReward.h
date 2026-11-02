// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgHitReward.h"
#include "Data/AgTaskParams.h"
#include "GameplayTagContainer.h"
#include "AgAbilityTask_HitReward.generated.h"

USTRUCT()
struct FAgHitRewardParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FAgHitReward> Rewards;

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

UCLASS()
class UAgAbilityTask_HitReward : public UAbilityTask
{
	GENERATED_BODY()
	friend struct FAgHitRewardParams;

public:
	virtual void Activate() override;

private:
	UPROPERTY()
	FAgHitRewardParams Params;

	FGameplayTagContainer AttackTags;
};
