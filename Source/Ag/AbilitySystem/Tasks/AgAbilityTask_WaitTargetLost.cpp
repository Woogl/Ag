// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_WaitTargetLost.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystem/AgTargeting.h"
#include "Character/AgCharacterBase.h"

UAbilityTask* FAgWaitTargetLostParams::MakeTask(const FAgTaskContext& Context) const
{
	UAgAbilityTask_WaitTargetLost* Task = UAbilityTask::NewAbilityTask<UAgAbilityTask_WaitTargetLost>(&Context.Ability);
	Task->Params = *this;
	return Task;
}

UAgAbilityTask_WaitTargetLost::UAgAbilityTask_WaitTargetLost()
{
	bTickingTask = true;
}

void UAgAbilityTask_WaitTargetLost::Activate()
{
	const UAgGameplayAbility* AgAbility = CastChecked<UAgGameplayAbility>(Ability);
	Target = Cast<AAgCharacterBase>(AgAbility->GetActionTarget());
	ReleaseRange = Params.ReleaseRange.Get(AgAbility->GetTargetingRange());
	if (IsTargetLost())
	{
		EndTask();
	}
}

void UAgAbilityTask_WaitTargetLost::TickTask(float DeltaTime)
{
	if (IsTargetLost())
	{
		EndTask();
	}
}

bool UAgAbilityTask_WaitTargetLost::IsTargetLost() const
{
	const AActor* Owner = GetAvatarActor();
	const AAgCharacterBase* Current = Target.Get();
	return !Owner || !Current || Current->IsDead() || !AgTargeting::IsInRange(*Owner, *Current, ReleaseRange);
}
