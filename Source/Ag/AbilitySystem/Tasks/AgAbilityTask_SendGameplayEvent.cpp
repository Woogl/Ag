// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_SendGameplayEvent.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"

UAbilityTask* FAgSendGameplayEventParams::MakeTask(const FAgTaskContext& Context) const
{
	UAgAbilityTask_SendGameplayEvent* Task = UAbilityTask::NewAbilityTask<UAgAbilityTask_SendGameplayEvent>(&Context.Ability);
	Task->Params = *this;
	return Task;
}

void UAgAbilityTask_SendGameplayEvent::Activate()
{
	const UAgGameplayAbility* AgAbility = Cast<UAgGameplayAbility>(Ability);
	AActor* Owner = GetAvatarActor();
	AActor* Target = AgAbility ? AgAbility->GetActionTarget() : nullptr;
	AActor* Receiver = Params.To == EAgSendGameplayEventTo::Owner ? Owner : Target;
	if (Owner && Receiver)
	{
		FGameplayEventData Payload;
		Payload.EventTag = Params.Event;
		Payload.Instigator = Owner;
		Payload.Target = Target;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Receiver, Params.Event, Payload);
	}
	EndTask();
}
