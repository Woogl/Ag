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

public:
	static UAgAbilityTask_Transition* Transition(UGameplayAbility* Owner, const FAgTransitionParams& Params, UAgAbilityTask_PlayMontage* Playback);

	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	TOptional<float> FindTagEndingDelay(const UAbilitySystemComponent& ASC) const;

	void TryTransition();
	void HandleEvent(const FGameplayEventData* Payload);
	void HandleTagChanged(const FGameplayTag Tag, int32 NewCount);

	FAgTransitionParams Params;
	TWeakObjectPtr<UAgAbilityTask_PlayMontage> Playback;
	FDelegateHandle EventHandle;
	FDelegateHandle TagHandle;
	FTimerHandle EndingTimer;
};
