// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_DisableRootMotion.generated.h"

USTRUCT()
struct FAgDisableRootMotionParams : public FAgTaskParams
{
	GENERATED_BODY()

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

/** 구간 동안 몽타주의 루트 모션을 버립니다. 엔진은 끈 동안에도 포즈의 루트 뼈를 고정하므로 모션은 제자리에서 재생됩니다. */
UCLASS()
class UAgAbilityTask_DisableRootMotion : public UAbilityTask
{
	GENERATED_BODY()
	friend struct FAgDisableRootMotionParams;

public:
	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	bool bMontageRootMotionDisabled = false;
};
