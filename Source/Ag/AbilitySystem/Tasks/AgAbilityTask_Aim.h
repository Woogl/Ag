// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_Aim.generated.h"

USTRUCT()
struct FAgAimParams : public FAgTaskParams
{
	GENERATED_BODY()

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

/** 구간 동안 행동 대상의 캡슐 중심을 어빌리티의 조준점으로 갱신합니다. 구간이 끝나면 마지막 조준점이 남습니다. */
UCLASS()
class UAgAbilityTask_Aim : public UAbilityTask
{
	GENERATED_BODY()

public:
	UAgAbilityTask_Aim();

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;

private:
	void UpdateAimLocation();
};
