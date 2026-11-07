// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgAbilityData.h"
#include "Data/AgTaskParams.h"
#include "GameplayTagContainer.h"
#include "AgAbilityTask_SendGameplayEvent.generated.h"

USTRUCT()
struct FAgSendGameplayEventParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag Event;

	/** 받을 캐릭터를 찾는 조건입니다. 비우면 행동의 대상(처형 대상 등) 하나에게 보냅니다. */
	UPROPERTY()
	FAgTargetingRequest Targets;

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

UCLASS()
class UAgAbilityTask_SendGameplayEvent : public UAbilityTask
{
	GENERATED_BODY()
	friend struct FAgSendGameplayEventParams;

public:
	virtual void Activate() override;

private:
	UPROPERTY()
	FAgSendGameplayEventParams Params;
};
