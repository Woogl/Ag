// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_TrackTarget.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystem/AgTargeting.h"
#include "Character/AgCharacterBase.h"

UAbilityTask* FAgTrackTargetParams::MakeTask(const FAgTaskContext& Context) const
{
	UAgAbilityTask_TrackTarget* Task = UAbilityTask::NewAbilityTask<UAgAbilityTask_TrackTarget>(&Context.Ability);
	Task->Params = *this;
	return Task;
}

UAgAbilityTask_TrackTarget::UAgAbilityTask_TrackTarget()
{
	bTickingTask = true;
}

void UAgAbilityTask_TrackTarget::Activate()
{
	const UAgGameplayAbility* AgAbility = CastChecked<UAgGameplayAbility>(Ability);
	ReleaseRange = Params.ReleaseRange.Get(AgAbility->GetTargetingRange());
	if (!UpdateTarget())
	{
		EndTask();
	}
}

void UAgAbilityTask_TrackTarget::TickTask(float DeltaTime)
{
	if (!UpdateTarget())
	{
		EndTask();
	}
}

bool UAgAbilityTask_TrackTarget::UpdateTarget()
{
	const AActor* Owner = GetAvatarActor();
	UAgGameplayAbility* AgAbility = CastChecked<UAgGameplayAbility>(Ability);
	const AAgCharacterBase* Current = Cast<AAgCharacterBase>(AgAbility->GetActionTarget());
	if (Current && Current->IsDead())
	{
		AgAbility->RefreshTarget();
		Current = Cast<AAgCharacterBase>(AgAbility->GetActionTarget());
	}
	return Owner && Current && !Current->IsDead() && AgTargeting::IsInRange(*Owner, *Current, ReleaseRange);
}
