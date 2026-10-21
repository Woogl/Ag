// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_WaitTargetLost.generated.h"

class AAgCharacterBase;

USTRUCT()
struct FAgWaitTargetLostParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	TOptional<float> ReleaseRange;

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

UCLASS()
class UAgAbilityTask_WaitTargetLost : public UAbilityTask
{
	GENERATED_BODY()
	friend struct FAgWaitTargetLostParams;

public:
	UAgAbilityTask_WaitTargetLost();

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;

private:
	bool IsTargetLost() const;

	UPROPERTY()
	FAgWaitTargetLostParams Params;

	TWeakObjectPtr<AAgCharacterBase> Target;
	float ReleaseRange = 0.f;
};
