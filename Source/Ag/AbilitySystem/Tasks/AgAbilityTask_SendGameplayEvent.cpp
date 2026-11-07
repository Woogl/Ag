// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_SendGameplayEvent.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystem/AgTargeting.h"
#include "AbilitySystemBlueprintLibrary.h"

UAbilityTask* FAgSendGameplayEventParams::MakeTask(const FAgTaskContext& Context) const
{
	UAgAbilityTask_SendGameplayEvent* Task = UAbilityTask::NewAbilityTask<UAgAbilityTask_SendGameplayEvent>(&Context.Ability);
	Task->Params = *this;
	return Task;
}

void UAgAbilityTask_SendGameplayEvent::Activate()
{
	if (AActor* Owner = GetAvatarActor())
	{
		for (AActor* Target : AgTargeting::FindTargets(Params.Targets, *Owner))
		{
			FGameplayEventData Payload;
			Payload.EventTag = Params.Event;
			Payload.Instigator = Owner;
			Payload.Target = Target;
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Target, Params.Event, Payload);
		}
	}
	EndTask();
}
