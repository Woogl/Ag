// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_WaitMontagePosition.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

UAgAbilityTask_WaitMontagePosition::UAgAbilityTask_WaitMontagePosition()
{
	bTickingTask = true;
}

UAgAbilityTask_WaitMontagePosition* UAgAbilityTask_WaitMontagePosition::WaitMontagePosition(UGameplayAbility* OwningAbility, UAnimMontage* Montage, float Position)
{
	UAgAbilityTask_WaitMontagePosition* Task = NewAbilityTask<UAgAbilityTask_WaitMontagePosition>(OwningAbility);
	Task->Montage = Montage;
	Task->Position = Position;
	return Task;
}

void UAgAbilityTask_WaitMontagePosition::Activate()
{
	CheckPosition();
}

void UAgAbilityTask_WaitMontagePosition::TickTask(float DeltaTime)
{
	CheckPosition();
}

void UAgAbilityTask_WaitMontagePosition::CheckPosition()
{
	if (!IsValid(this) || IsFinished())
	{
		return;
	}

	const UAnimInstance* AnimInstance = Ability ? Ability->GetCurrentActorInfo()->GetAnimInstance() : nullptr;
	const bool bPlaying = AnimInstance && Montage.IsValid() && AnimInstance->Montage_IsActive(Montage.Get());
	if (!bPlaying || AnimInstance->Montage_GetPosition(Montage.Get()) >= Position)
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnReached.Broadcast();
		}
		EndTask();
	}
}
