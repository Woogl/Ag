// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "ActiveGameplayEffectHandle.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_ApplyEffect.generated.h"

class UGameplayEffect;

USTRUCT()
struct FAgApplyEffectParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY(meta = (DataId = "Effects"))
	FName Effect;

	UPROPERTY()
	bool bRemoveOnEnd = false;

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

UCLASS()
class UAgAbilityTask_ApplyEffect : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UAgAbilityTask_ApplyEffect* ApplyEffect(UGameplayAbility* Owner, FName Effect, bool bRemoveOnEnd);

	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	FName Effect;
	bool bRemoveOnEnd = false;
	FActiveGameplayEffectHandle Applied;
};
