// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_SendGameplayEvent.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"

void FAgSendGameplayEventParams::Validate(const FJsonObject& Json, EAgTaskPlacement Placement, const FString& Path, TArray<FString>& Errors)
{
	if (!Event.ToString().StartsWith(TEXT("Event.")))
	{
		Errors.Add(Path + TEXT(".Event: expected a tag under Event"));
	}
}

UAbilityTask* FAgSendGameplayEventParams::MakeTask(const FAgTaskContext& Context) const
{
	return UAgAbilityTask_SendGameplayEvent::SendGameplayEvent(&Context.Runner, *this);
}

UAgAbilityTask_SendGameplayEvent* UAgAbilityTask_SendGameplayEvent::SendGameplayEvent(UGameplayAbility* Owner,
                                                                                      const FAgSendGameplayEventParams& Params)
{
	UAgAbilityTask_SendGameplayEvent* Task = NewAbilityTask<UAgAbilityTask_SendGameplayEvent>(Owner);
	Task->Event = Params.Event;
	Task->Recipient = Params.Recipient;
	return Task;
}

void UAgAbilityTask_SendGameplayEvent::Activate()
{
	const UAgGameplayAbility* Runner = Cast<UAgGameplayAbility>(Ability);
	AActor* Owner = GetAvatarActor();
	AActor* Target = Runner ? Runner->GetActionTarget() : nullptr;
	AActor* Receiver = Recipient == EAgEventRecipient::Owner ? Owner : Target;
	if (Owner && Receiver)
	{
		FGameplayEventData Payload;
		Payload.EventTag = Event;
		Payload.Instigator = Owner;
		Payload.Target = Target;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Receiver, Event, Payload);
	}
	if (!IsFinished())
	{
		EndTask();
	}
}
