// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_WaitTargetLost.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "Character/AgCharacterBase.h"

UAbilityTask* FAgWaitTargetLostParams::MakeTask(const FAgTaskContext& Context) const
{
	return UAgAbilityTask_WaitTargetLost::WaitTargetLost(&Context.Ability, *this);
}

UAgAbilityTask_WaitTargetLost::UAgAbilityTask_WaitTargetLost()
{
	bTickingTask = true;
}

UAgAbilityTask_WaitTargetLost* UAgAbilityTask_WaitTargetLost::WaitTargetLost(UGameplayAbility* Owner, const FAgWaitTargetLostParams& InParams)
{
	UAgAbilityTask_WaitTargetLost* Task = NewAbilityTask<UAgAbilityTask_WaitTargetLost>(Owner);
	Task->Params = InParams;
	return Task;
}

void UAgAbilityTask_WaitTargetLost::Activate()
{
	const UAgGameplayAbility* AgAbility = Cast<UAgGameplayAbility>(Ability);
	Target = AgAbility ? Cast<AAgCharacterBase>(AgAbility->GetActionTarget()) : nullptr;
	Params.ReleaseRange = Params.ReleaseRange.Get(AgAbility ? AgAbility->GetTargetingRange() : 0.f);
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
	const float Range = Params.ReleaseRange.Get(0.f);
	return !Owner || !Current || Current->IsDead() || Range <= 0.f || Owner->GetHorizontalDistanceTo(Current) > Range;
}
