// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgTaskParams.h"
#include "GameplayTagContainer.h"
#include "AgAbilityTask_Transition.generated.h"

class UAbilitySystemComponent;
struct FGameplayEventData;

USTRUCT()
struct FAgTransitionParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	FAgTransition Next;

	UPROPERTY()
	FGameplayTagContainer RequiredTags;

	UPROPERTY()
	FGameplayTagContainer BlockedTags;

	UPROPERTY()
	FGameplayTag Event;

	UPROPERTY()
	FGameplayTag BeforeTagEnds;

	UPROPERTY()
	bool bOptional = false;

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

UCLASS()
class UAgAbilityTask_Transition : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UAgAbilityTask_Transition* Transition(UGameplayAbility* Owner, const FAgTransitionParams& Params, bool bOnce, int32 MontageInstanceId);

	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	bool AreConditionsMet() const;

	void WatchTags(UAbilitySystemComponent& ASC, const FGameplayTagContainer& Tags);

	void TryTransition();
	void HandleEvent(const FGameplayEventData* Payload);
	void HandleTagChanged(const FGameplayTag Tag, int32 NewCount);
	void HandleTagEnding();

	FAgTransitionParams Params;
	int32 MontageInstanceId = INDEX_NONE;

	bool bOnce = false;
	FDelegateHandle EventHandle;
	TArray<TPair<FGameplayTag, FDelegateHandle>> TagHandles;
	FTimerHandle EndingTimer;

	bool bTagEnding = false;
};
