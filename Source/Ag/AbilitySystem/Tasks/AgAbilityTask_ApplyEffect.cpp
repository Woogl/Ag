// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_ApplyEffect.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Core/AgAssetManager.h"
#include "GameplayEffect.h"
#include "Ag.h"

UAbilityTask* FAgApplyEffectParams::MakeTask(const FAgTaskContext& Context) const
{
	UAgAbilityTask_ApplyEffect* Task = UAbilityTask::NewAbilityTask<UAgAbilityTask_ApplyEffect>(&Context.Ability);
	Task->Params = *this;
	return Task;
}

void UAgAbilityTask_ApplyEffect::Activate()
{
	UAgGameplayAbility* AgAbility = Cast<UAgGameplayAbility>(Ability);
	const UGameplayEffect* Definition = UAgAssetManager::Get().FindEffect(Params.Effect);
	if (!AgAbility || !Definition || !AbilitySystemComponent.IsValid())
	{
		UE_LOG(LogAg, Error, TEXT("ApplyEffect: effect %s is missing in Effects"), *Params.Effect.ToString());
		EndTask();
		return;
	}
	const FGameplayEffectSpecHandle Spec = AgAbility->MakeEffectSpec(*Definition);
	if (!Spec.IsValid())
	{
		EndTask();
		return;
	}
	// 효과가 주는 태그의 알림이 이 구간을 끝낼 수 있습니다. 거는 동안 이미 끝났으면(구간이 닫힘) 걸린 효과를 여기서
	// 지웁니다.
	const FActiveGameplayEffectHandle Handle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data);
	if (IsFinished())
	{
		if (Params.bRemoveOnEnd && Handle.IsValid() && AbilitySystemComponent.IsValid())
		{
			AbilitySystemComponent->RemoveActiveGameplayEffect(Handle);
		}
		return;
	}
	Applied = Handle;
	if (!Params.bRemoveOnEnd)
	{
		EndTask();
	}
}

void UAgAbilityTask_ApplyEffect::OnDestroy(bool bInOwnerFinished)
{
	if (Params.bRemoveOnEnd && Applied.IsValid() && AbilitySystemComponent.IsValid())
	{
		AbilitySystemComponent->RemoveActiveGameplayEffect(Applied);
	}
	Super::OnDestroy(bInOwnerFinished);
}
