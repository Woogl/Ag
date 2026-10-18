// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgTaskParams.h"
#include "GameplayTagContainer.h"
#include "AgAbilityTask_Transition.generated.h"

class UAbilitySystemComponent;
struct FGameplayEventData;

/**
 * 조건이 맞으면 전환합니다(Transition Task의 Params). 주인이 RequiredTags를 모두 갖고 BlockedTags를 하나도 갖지 않을 때, 지점이면
 * 그 시각에, 발동이면 재생 전에(시작 섹션), 구간이면 주인이 Event를 받을 때나 BeforeTagEnds의 태그를 주는 효과가 끝나기 Next 섹션
 * 길이만큼 전에(이미 지났으면 바로, 끝나는 때를 모르는 태그면 사라질 때, 그때 조건이 맞지 않으면 맞게 될 때) 전환합니다. 구간에
 * 둘 다 없으면 구간 안에서 조건이 맞게 되는 때입니다.
 */
USTRUCT()
struct FAgTransitionParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	FAgTransition Next;

	/** 선택 */
	UPROPERTY()
	FGameplayTagContainer RequiredTags;

	/** 선택 */
	UPROPERTY()
	FGameplayTagContainer BlockedTags;

	/** 선택. 구간만 씁니다. */
	UPROPERTY()
	FGameplayTag Event;

	/** 선택. 구간만 씁니다. */
	UPROPERTY()
	FGameplayTag BeforeTagEnds;

	/** 선택. 구간만 씁니다. 전환하지 않고 구간이 끝나도 되는 전환이라, 반복 섹션을 덮지 않아도 됩니다(검증). */
	UPROPERTY()
	bool bOptional = false;

	virtual EAgTaskPlacement GetPlacements() const override
	{
		return EAgTaskPlacement::Activation | EAgTaskPlacement::Window | EAgTaskPlacement::Point;
	}

	virtual void GetFieldRules(FAgJsonFieldRules& Rules) const override;
	virtual void Validate(const FJsonObject& Json, EAgTaskPlacement Placement, const FString& Path, TArray<FString>& Errors) override;

	virtual const FAgTransition* GetTransition() const override
	{
		return &Next;
	}

	virtual bool WaitsForTransition() const override
	{
		return !bOptional;
	}

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

/** 정한 때에 조건을 확인해 Runner에 전환을 요청합니다. 지점과 발동은 시작할 때 한 번 보고 끝나고, 구간은 끝날 때 Runner가 끝냅니다. */
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

	/** Tags가 붙거나 떨어질 때 HandleTagChanged를 부릅니다. */
	void WatchTags(UAbilitySystemComponent& ASC, const FGameplayTagContainer& Tags);

	/** 조건이 맞으면 전환을 요청합니다. */
	void TryTransition();
	void HandleEvent(const FGameplayEventData* Payload);
	void HandleTagChanged(const FGameplayTag Tag, int32 NewCount);
	void HandleTagEnding();

	FAgTransitionParams Params;
	int32 MontageInstanceId = INDEX_NONE;

	/** 시작할 때 한 번 보고 끝납니다(지점, 발동). */
	bool bOnce = false;
	FDelegateHandle EventHandle;
	TArray<TPair<FGameplayTag, FDelegateHandle>> TagHandles;
	FTimerHandle EndingTimer;

	/** BeforeTagEnds: 전환할 때가 됐습니다(남은 시간이 Next 섹션 길이 이하이거나 태그가 사라짐). */
	bool bTagEnding = false;
};
