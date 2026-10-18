// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgTaskParams.h"
#include "GameplayTagContainer.h"
#include "AgAbilityTask_SendGameplayEvent.generated.h"

/** SendGameplayEvent의 이벤트를 받는 쪽 */
UENUM()
enum class EAgEventRecipient : uint8
{
	Owner,

	/** 행동 대상 */
	Target
};

/** 게임플레이 이벤트 하나(SendGameplayEvent Task의 Params). Instigator는 주인, Target은 행동 대상입니다. */
USTRUCT()
struct FAgSendGameplayEventParams : public FAgTaskParams
{
	GENERATED_BODY()

	/** Event 아래의 태그 */
	UPROPERTY()
	FGameplayTag Event;

	UPROPERTY()
	EAgEventRecipient Recipient = EAgEventRecipient::Owner;

	virtual EAgTaskPlacement GetPlacements() const override
	{
		return EAgTaskPlacement::Activation | EAgTaskPlacement::Point;
	}

	virtual void Validate(const FJsonObject& Json, EAgTaskPlacement Placement, const FString& Path, TArray<FString>& Errors) override;

	virtual bool NeedsTarget() const override
	{
		return Recipient == EAgEventRecipient::Target;
	}

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

/** 이벤트 하나를 보내고 끝납니다. 받는 쪽이 없으면(대상 없음) 보내지 않습니다. */
UCLASS()
class UAgAbilityTask_SendGameplayEvent : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UAgAbilityTask_SendGameplayEvent* SendGameplayEvent(UGameplayAbility* Owner, const FAgSendGameplayEventParams& Params);

	virtual void Activate() override;

private:
	FGameplayTag Event;
	EAgEventRecipient Recipient = EAgEventRecipient::Owner;
};
