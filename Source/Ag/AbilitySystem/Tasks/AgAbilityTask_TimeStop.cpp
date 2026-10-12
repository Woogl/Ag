// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_TimeStop.h"

#include "Character/AgCharacterBase.h"
#include "Combat/AgTimeSubsystem.h"
#include "Engine/World.h"

UAgAbilityTask_TimeStop* UAgAbilityTask_TimeStop::TimeStop(UGameplayAbility* OwningAbility)
{
	return NewAbilityTask<UAgAbilityTask_TimeStop>(OwningAbility);
}

void UAgAbilityTask_TimeStop::Activate()
{
	Super::Activate();

	AAgCharacterBase* Avatar = Cast<AAgCharacterBase>(GetAvatarActor());
	if (!Avatar)
	{
		EndTask();
		return;
	}
	UAgTimeSubsystem* Subsystem = GetWorld()->GetSubsystem<UAgTimeSubsystem>();
	TimeSubsystem = Subsystem;
	Subsystem->StartTimeStop(*Avatar);
}

void UAgAbilityTask_TimeStop::OnDestroy(bool bInOwnerFinished)
{
	if (UAgTimeSubsystem* Subsystem = TimeSubsystem.Get())
	{
		Subsystem->EndTimeStop();
	}

	Super::OnDestroy(bInOwnerFinished);
}
