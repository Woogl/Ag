// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_HoldTag.h"

#include "AbilitySystemComponent.h"

UAgAbilityTask_HoldTag* UAgAbilityTask_HoldTag::HoldTag(UGameplayAbility* OwningAbility, const FGameplayTag& Tag)
{
	UAgAbilityTask_HoldTag* Task = NewAbilityTask<UAgAbilityTask_HoldTag>(OwningAbility);
	Task->Tag = Tag;
	return Task;
}

void UAgAbilityTask_HoldTag::Activate()
{
	if (AbilitySystemComponent.IsValid() && Tag.IsValid())
	{
		AbilitySystemComponent->AddLooseGameplayTag(Tag);
		bAdded = true;
	}
}

void UAgAbilityTask_HoldTag::OnDestroy(bool bInOwnerFinished)
{
	if (bAdded && AbilitySystemComponent.IsValid())
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(Tag);
		bAdded = false;
	}

	Super::OnDestroy(bInOwnerFinished);
}
