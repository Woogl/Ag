// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgAbilityData.h"
#include "Data/AgAttackData.h"
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

	/** 비우지 않으면 판정하는 순간 이 큐를 발밑에서 실행합니다. RawMagnitude는 판정 반경입니다. */
	UPROPERTY()
	FGameplayTag Cue;

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
