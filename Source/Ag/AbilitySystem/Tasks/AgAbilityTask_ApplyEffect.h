// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "ActiveGameplayEffectHandle.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_ApplyEffect.generated.h"

class UGameplayEffect;

/**
 * 주인에게 거는 효과(ApplyEffect Task의 Params). Effect는 Effects.json의 효과 ID입니다. 구간이면 구간 시작에 걸고 끝에 지우고(모션
 * 시간), 발동·지점이면 효과의 지속 시간(월드 시간)을 따르되 bRemoveOnEnd면 행동이 끝날 때 지웁니다.
 */
USTRUCT()
struct FAgApplyEffectParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	FName Effect;

	/** 선택. 발동·지점만 씁니다. */
	UPROPERTY()
	bool bRemoveOnEnd = false;

	/** Effect가 Effects.json에 있는지, 구간이면 무한 효과인지 검사합니다. */
	void ValidateEffect(const TMap<FName, TObjectPtr<UGameplayEffect>>& Effects, EAgTaskPlacement Placement, const FString& Path, TArray<FString>& Errors) const;

	virtual EAgTaskPlacement GetPlacements() const override
	{
		return EAgTaskPlacement::Activation | EAgTaskPlacement::Window | EAgTaskPlacement::Point;
	}

	virtual void GetFieldRules(FAgJsonFieldRules& Rules) const override;
	virtual void Validate(const FJsonObject& Json, EAgTaskPlacement Placement, const FString& Path, TArray<FString>& Errors) override;
	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

/** 효과 하나를 행동의 효과 컨텍스트로 주인에게 겁니다. bRemoveOnEnd면 이 Task가 끝날 때 지우고, 아니면 걸고 바로 끝납니다. */
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
