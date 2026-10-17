// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_HoldTag.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystemComponent.h"

UAbilityTask* FAgHoldTagParams::MakeTask(const FAgTaskContext& Context) const
{
	return UAgAbilityTask_HoldTag::HoldTag(&Context.Runner, Tag);
}

UAgAbilityTask_HoldTag* UAgAbilityTask_HoldTag::HoldTag(UGameplayAbility* OwningAbility, const FGameplayTag& Tag)
{
	UAgAbilityTask_HoldTag* Task = NewAbilityTask<UAgAbilityTask_HoldTag>(OwningAbility);
	Task->Tag = Tag;
	return Task;
}

void UAgAbilityTask_HoldTag::Activate()
{
	const bool bOwnerWasActive = Ability && Ability->IsActive();
	if (AbilitySystemComponent.IsValid() && Tag.IsValid())
	{
		bAdded = true;
		AbilitySystemComponent->AddLooseGameplayTag(Tag);
	}
	// 활성 Task 목록 등록 전 태그 알림이 소유 Ability를 취소할 수 있습니다.
	if (bOwnerWasActive && !IsFinished() && (!Ability || !Ability->IsActive()))
	{
		EndTask();
	}
}

void UAgAbilityTask_HoldTag::OnDestroy(bool bInOwnerFinished)
{
	if (bAdded && AbilitySystemComponent.IsValid())
	{
		bAdded = false;
		AbilitySystemComponent->RemoveLooseGameplayTag(Tag);
	}

	Super::OnDestroy(bInOwnerFinished);
}
