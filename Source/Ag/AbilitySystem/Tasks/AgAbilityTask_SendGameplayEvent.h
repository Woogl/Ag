// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgTaskParams.h"
#include "GameplayTagContainer.h"
#include "AgAbilityTask_SendGameplayEvent.generated.h"

UENUM()
enum class EAgEventRecipient : uint8
{
	Owner,
	Target
};

USTRUCT()
struct FAgSendGameplayEventParams : public FAgTaskParams
{
	GENERATED_BODY()

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
