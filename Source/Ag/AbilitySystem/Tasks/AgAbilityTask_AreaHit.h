// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgAbilityData.h"
#include "Data/AgAttackData.h"
#include "Data/AgHitReward.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_AreaHit.generated.h"

USTRUCT()
struct FAgAreaHitParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	FAgDamage Damage;

	UPROPERTY()
	FAgTargetingRequest Area;

	UPROPERTY()
	TOptional<FAgHitReward> HitReward;

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

UCLASS()
class UAgAbilityTask_AreaHit : public UAbilityTask
{
	GENERATED_BODY()
	friend struct FAgAreaHitParams;

public:
	virtual void Activate() override;

private:
	UPROPERTY()
	FAgAreaHitParams Params;
};
