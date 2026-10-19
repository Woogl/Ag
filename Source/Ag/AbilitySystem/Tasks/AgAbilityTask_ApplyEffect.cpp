// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_ApplyEffect.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Core/AgAssetManager.h"
#include "GameplayEffect.h"
#include "Ag.h"

UAbilityTask* FAgApplyEffectParams::MakeTask(const FAgTaskContext& Context) const
{
	return UAgAbilityTask_ApplyEffect::ApplyEffect(&Context.Runner, Effect, Context.Placement == EAgTaskPlacement::Window || bRemoveOnEnd);
}

UAgAbilityTask_ApplyEffect* UAgAbilityTask_ApplyEffect::ApplyEffect(UGameplayAbility* Owner, FName InEffect, bool bInRemoveOnEnd)
{
	UAgAbilityTask_ApplyEffect* Task = NewAbilityTask<UAgAbilityTask_ApplyEffect>(Owner);
	Task->Effect = InEffect;
	Task->bRemoveOnEnd = bInRemoveOnEnd;
	return Task;
}

void UAgAbilityTask_ApplyEffect::Activate()
{
	UAgGameplayAbility* Runner = Cast<UAgGameplayAbility>(Ability);
	const UGameplayEffect* Definition = UAgAssetManager::Get().FindEffect(Effect);
	if (!Runner || !Definition || !AbilitySystemComponent.IsValid())
	{
		UE_LOG(LogAg, Error, TEXT("ApplyEffect: effect %s is missing in Effects"), *Effect.ToString());
		EndTask();
		return;
	}
	const FGameplayEffectSpecHandle Spec = Runner->MakeEffectSpec(*Definition);
	if (!Spec.IsValid())
	{
		EndTask();
		return;
	}
	// 효과가 주는 태그의 알림이 행동이나 이 구간을 끝낼 수 있습니다. 시작 중인 Task는 아직 행동의 Task 목록에 없어
	// 행동이 끝내지 못하므로 스스로 끝내고, 거는 동안 이미 끝났으면(구간이 닫힘) 걸린 효과를 여기서 지웁니다.
	const bool bOwnerWasActive = Ability->IsActive();
	const FActiveGameplayEffectHandle Handle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data);
	if (IsFinished())
	{
		if (bRemoveOnEnd && Handle.IsValid() && AbilitySystemComponent.IsValid())
		{
			AbilitySystemComponent->RemoveActiveGameplayEffect(Handle);
		}
		return;
	}
	Applied = Handle;
	if (!bRemoveOnEnd || (bOwnerWasActive && !Ability->IsActive()))
	{
		EndTask();
	}
}

void UAgAbilityTask_ApplyEffect::OnDestroy(bool bInOwnerFinished)
{
	if (bRemoveOnEnd && Applied.IsValid() && AbilitySystemComponent.IsValid())
	{
		AbilitySystemComponent->RemoveActiveGameplayEffect(Applied);
	}
	Applied.Invalidate();
	Super::OnDestroy(bInOwnerFinished);
}
