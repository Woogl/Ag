// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgTaskParams.h"
#include "GameplayTagContainer.h"
#include "AgAbilityTask_HoldTag.generated.h"

USTRUCT()
struct FAgHoldTagParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag Tag;

	virtual EAgTaskPlacement GetPlacements() const override
	{
		return EAgTaskPlacement::Window | EAgTaskPlacement::Point;
	}

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

UCLASS()
class UAgAbilityTask_HoldTag : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UAgAbilityTask_HoldTag* HoldTag(UGameplayAbility* OwningAbility, const FGameplayTag& Tag);

	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	FGameplayTag Tag;
	bool bAdded = false;
};
