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

	UPROPERTY()
	FName Effect;

	UPROPERTY()
	bool bRemoveOnEnd = false;

	virtual EAgTaskPlacement GetPlacements() const override
	{
		return EAgTaskPlacement::Activation | EAgTaskPlacement::Window | EAgTaskPlacement::Point;
	}

	virtual void GetFieldRules(FAgJsonFieldRules& Rules) const override;
	virtual void Validate(const FJsonObject& Json, EAgTaskPlacement Placement, const FString& Path, TArray<FString>& Errors) override;

	virtual void GetReferences(FAgTaskReferences& Out) const override
	{
		Out.Effects.Add(Effect);
	}

	virtual void ValidateLinks(const FAgTaskLinks& Links, const FString& Path, TArray<FString>& Errors) const override;
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
