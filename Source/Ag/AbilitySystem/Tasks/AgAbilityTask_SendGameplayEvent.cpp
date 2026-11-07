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
		TArray<AActor*> Targets;
		if (Params.Targets.Range > 0.f)
		{
			Targets = AgTargeting::FindTargets(Params.Targets, *Owner);
		}
		else if (AActor* ActionTarget = CastChecked<UAgGameplayAbility>(Ability)->GetActionTarget())
		{
			Targets.Add(ActionTarget);
		}
		for (AActor* Target : Targets)
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
