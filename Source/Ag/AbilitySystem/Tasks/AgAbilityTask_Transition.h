// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgTaskParams.h"
#include "GameplayTagContainer.h"
#include "AgAbilityTask_Transition.generated.h"

class UAbilitySystemComponent;
class UAgAbilityTask_PlayMontage;
struct FGameplayEventData;

USTRUCT()
struct FAgTransitionParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	FName ToSection;

	UPROPERTY()
	TOptional<float> BlendTime;

	UPROPERTY()
	FGameplayTag Event;

	UPROPERTY()
	FGameplayTag TagRemoved;

	/** TagRemoved와 함께 켜면, 그 태그를 주는 효과가 끝나기 ToSection 길이만큼 전에 미리 넘어가 그 섹션이 효과와 함께 끝납니다. */
	UPROPERTY()
	bool bSectionEndsWithTag = false;

	/** 켜면 옮기기 전에 어빌리티를 다시 커밋(비용·쿨다운)하고, 커밋하지 못하면 옮기지 않고 계속 기다립니다. */
	UPROPERTY()
	bool bCommitAbility = false;

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

UCLASS()
class UAgAbilityTask_Transition : public UAbilityTask
{
	GENERATED_BODY()
	friend struct FAgTransitionParams;

public:
	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	TOptional<float> FindTagEndingDelay(const UAbilitySystemComponent& ASC) const;

	/** 주인에게 태그가 있으면 사라지기를 기다리기 시작하고 true를 돌려줍니다. */
	bool WaitForTag();
	void RecheckTag();

	void TryTransition();
	void HandleEvent(const FGameplayEventData* Payload);
	void HandleTagChanged(const FGameplayTag Tag, int32 NewCount);

	UPROPERTY()
	FAgTransitionParams Params;

	TWeakObjectPtr<UAgAbilityTask_PlayMontage> Playback;
	FDelegateHandle EventHandle;
	FDelegateHandle TagHandle;
	FTimerHandle EndingTimer;
};
