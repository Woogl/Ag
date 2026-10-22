// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "ActiveGameplayEffectHandle.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_ApplyEffect.generated.h"

class UGameplayEffect;
struct FGameplayEffectRemovalInfo;

USTRUCT()
struct FAgApplyEffectParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY(meta = (DataId = "Effects"))
	FName Effect;

	UPROPERTY()
	bool bRemoveOnEnd = true;

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

UCLASS()
class UAgAbilityTask_ApplyEffect : public UAbilityTask
{
	GENERATED_BODY()
	friend struct FAgApplyEffectParams;

public:
	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	void HandleEffectRemoved(const FGameplayEffectRemovalInfo& RemovalInfo);

	UPROPERTY()
	FAgApplyEffectParams Params;

	FActiveGameplayEffectHandle Applied;
	FDelegateHandle RemovedHandle;
};
