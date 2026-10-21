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
	FName Section;

	UPROPERTY()
	TOptional<float> BlendTime;

	UPROPERTY()
	FGameplayTag Event;

	UPROPERTY()
	FGameplayTag Tag;

	UPROPERTY()
	bool bSectionEndsWithTag = false;

	UPROPERTY()
	FGameplayTagContainer RequiredTags;

	UPROPERTY()
	FGameplayTagContainer BlockedTags;

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
