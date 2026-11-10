// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_HitReward.h"

#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "Data/AgEffectData.h"
#include "Ag.h"

UAbilityTask* FAgHitRewardParams::MakeTask(const FAgTaskContext& Context) const
{
	const FGameplayEventData* TriggerEvent = Context.Ability.GetTriggerEvent();
	if (!TriggerEvent)
	{
		UE_LOG(LogAg, Error, TEXT("HitReward (ability %s): place it in Tasks of an ability triggered by Event.AttackHit"),
			*Context.Ability.GetAbilityId().ToString());
		return nullptr;
	}
	UAgAbilityTask_HitReward* Task = UAbilityTask::NewAbilityTask<UAgAbilityTask_HitReward>(&Context.Ability);
	Task->Params = *this;
	Task->AttackTags = TriggerEvent->InstigatorTags;
	return Task;
}

void UAgAbilityTask_HitReward::Activate()
{
	UAgAbilitySystemComponent* ASC = CastChecked<UAgAbilitySystemComponent>(AbilitySystemComponent.Get());
	const FName Section = ASC->GetCurrentMontageSectionName();
	for (const FAgHitReward& Reward : Params.Rewards)
	{
		if (!AttackTags.HasTag(Reward.AbilityTag) || (!Reward.Section.IsNone() && Reward.Section != Section))
		{
			continue;
		}
		const FGameplayEffectSpecHandle Spec = ASC->MakeEffectSpec(EAgEffect::Resource, 1.f, FGameplayEffectContextHandle());
		if (Spec.IsValid())
		{
			Reward.AddToSpec(*Spec.Data);
			ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
		}
	}
	EndTask();
}
