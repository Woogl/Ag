// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_UseItem.generated.h"

/** 쓸 아이템은 어빌리티 정의의 Item입니다. */
USTRUCT()
struct FAgUseItemParams : public FAgTaskParams
{
	GENERATED_BODY()

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

UCLASS()
class UAgAbilityTask_UseItem : public UAbilityTask
{
	GENERATED_BODY()

public:
	virtual void Activate() override;
};
