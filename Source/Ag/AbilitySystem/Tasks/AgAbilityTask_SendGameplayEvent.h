// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgTaskParams.h"
#include "GameplayTagContainer.h"
#include "AgAbilityTask_SendGameplayEvent.generated.h"

USTRUCT()
struct FAgSendGameplayEventParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag Event;

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
