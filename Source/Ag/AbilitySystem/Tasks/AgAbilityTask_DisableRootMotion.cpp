// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_DisableRootMotion.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "Character/AgCharacterBase.h"

UAbilityTask* FAgDisableRootMotionParams::MakeTask(const FAgTaskContext& Context) const
{
	return UAbilityTask::NewAbilityTask<UAgAbilityTask_DisableRootMotion>(&Context.Ability);
}

void UAgAbilityTask_DisableRootMotion::Activate()
{
	if (AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor()))
	{
		bMontageRootMotionDisabled = true;
		Character->PushDisableMontageRootMotion();
	}
}

void UAgAbilityTask_DisableRootMotion::OnDestroy(bool bInOwnerFinished)
{
	AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor());
	if (bMontageRootMotionDisabled && Character)
	{
		Character->PopDisableMontageRootMotion();
	}
	Super::OnDestroy(bInOwnerFinished);
}
