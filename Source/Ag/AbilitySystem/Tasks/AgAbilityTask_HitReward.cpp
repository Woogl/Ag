// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_HitReward.h"

#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "Data/AgEffectData.h"
#include "Ag.h"

UAbilityTask* FAgHitRewardParams::MakeTask(const FAgTaskContext& Context) const
{
	UAgAbilityTask_HitReward* Task = UAbilityTask::NewAbilityTask<UAgAbilityTask_HitReward>(&Context.Ability);
	Task->Params = *this;
	return Task;
}

void UAgAbilityTask_HitReward::Activate()
{
	const UAgGameplayAbility* AgAbility = CastChecked<UAgGameplayAbility>(Ability);
	const FGameplayEventData* TriggerEvent = AgAbility->GetTriggerEvent();
	if (!TriggerEvent)
	{
		UE_LOG(LogAg, Error, TEXT("HitReward (ability %s): place it in Tasks of an ability triggered by Event.AttackHit"),
			*AgAbility->GetAbilityId().ToString());
		EndTask();
		return;
	}
	// 공격한 어빌리티의 태그입니다.
	const FGameplayTagContainer AttackTags = TriggerEvent->InstigatorTags;
	UAgAbilitySystemComponent* ASC = CastChecked<UAgAbilitySystemComponent>(AbilitySystemComponent.Get());
	const FName Section = ASC->GetCurrentMontageSectionName();
	for (const FAgHitReward& Reward : Params.Rewards)
	{
		if (!AttackTags.HasTag(Reward.AbilityTag) || (!Reward.Section.IsNone() && Reward.Section != Section))
		{
			continue;
		}
		const FGameplayEffectSpecHandle Spec = ASC->MakeEffectSpec(EAgEffect::Resource);
		Reward.AddToSpec(*Spec.Data);
		ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
	}
	EndTask();
}
