// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "ActiveGameplayEffectHandle.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_ApplyEffect.generated.h"

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

	/** 이 Task가 끝날 때 지울 효과입니다. 걸어 두고 끝나는 효과(즉시 효과, bRemoveOnEnd가 꺼진 효과)는 담지 않습니다. */
	FActiveGameplayEffectHandle Applied;
	FDelegateHandle RemovedHandle;
};
