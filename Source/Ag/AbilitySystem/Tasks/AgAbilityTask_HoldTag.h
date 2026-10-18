// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgTaskParams.h"
#include "GameplayTagContainer.h"
#include "AgAbilityTask_HoldTag.generated.h"

/**
 * 값 없는 상태 태그 하나(HoldTag Task의 Params). 구간이면 그동안 붙이고, 지점이면 그 시각부터 행동이 끝날 때까지 붙입니다. 행동
 * 내내 필요한 태그는 행동 정의 Activation의 OwnedTags에 둡니다.
 */
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

/** 도는 동안 주인에게 상태 태그 하나를 붙여 둡니다(예: 회피의 무적 구간). Runner가 구간 끝에 끝내거나 행동이 끝나면 뗍니다. */
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
