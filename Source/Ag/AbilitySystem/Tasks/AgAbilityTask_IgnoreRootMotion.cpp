// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_IgnoreRootMotion.h"

#include "Animation/AnimInstance.h"

UAgAbilityTask_IgnoreRootMotion* UAgAbilityTask_IgnoreRootMotion::IgnoreRootMotion(UGameplayAbility* OwningAbility)
{
	return NewAbilityTask<UAgAbilityTask_IgnoreRootMotion>(OwningAbility);
}

void UAgAbilityTask_IgnoreRootMotion::Activate()
{
	Super::Activate();

	UAnimInstance* Instance = Ability ? Ability->GetCurrentActorInfo()->GetAnimInstance() : nullptr;
	if (!Instance)
	{
		EndTask();
		return;
	}
	AnimInstance = Instance;
	SavedRootMotionMode = Instance->RootMotionMode;
	Instance->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
}

void UAgAbilityTask_IgnoreRootMotion::OnDestroy(bool bInOwnerFinished)
{
	if (UAnimInstance* Instance = AnimInstance.Get())
	{
		Instance->SetRootMotionMode(SavedRootMotionMode);
	}

	Super::OnDestroy(bInOwnerFinished);
}
