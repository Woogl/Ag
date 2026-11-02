// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_Aim.h"

#include "AbilitySystem/AgGameplayAbility.h"

UAbilityTask* FAgAimParams::MakeTask(const FAgTaskContext& Context) const
{
	return UAbilityTask::NewAbilityTask<UAgAbilityTask_Aim>(&Context.Ability);
}

UAgAbilityTask_Aim::UAgAbilityTask_Aim()
{
	bTickingTask = true;
}

void UAgAbilityTask_Aim::Activate()
{
	UpdateAimLocation();
}

void UAgAbilityTask_Aim::TickTask(float DeltaTime)
{
	UpdateAimLocation();
}

void UAgAbilityTask_Aim::UpdateAimLocation()
{
	UAgGameplayAbility* AgAbility = CastChecked<UAgGameplayAbility>(Ability);
	if (const AActor* Target = AgAbility->GetActionTarget())
	{
		AgAbility->SetAimLocation(Target->GetActorLocation());
	}
}
