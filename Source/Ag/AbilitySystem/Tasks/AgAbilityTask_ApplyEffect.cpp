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
	const UGameplayEffect* Definition = UAgAssetManager::Get().FindEffect(Params.Effect);
	if (!Definition)
	{
		UE_LOG(LogAg, Error, TEXT("ApplyEffect: effect %s is missing in Effects"), *Params.Effect.ToString());
		EndTask();
		return;
	}
	// 주인의 ASC가 없으면 스펙이 만들어지지 않습니다.
	const FGameplayEffectSpecHandle Spec = CastChecked<UAgGameplayAbility>(Ability)->MakeEffectSpec(*Definition);
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
	if (!Params.bRemoveOnEnd || Definition->DurationPolicy == EGameplayEffectDurationType::Instant)
	{
		EndTask();
		return;
	}
	// 효과가 지속 시간으로 먼저 사라진 뒤에도 Task가 남으면 End가 Tasks인 행동이 끝나지 않으므로 함께 끝냅니다.
	FOnActiveGameplayEffectRemoved_Info* Removed = AbilitySystemComponent->OnGameplayEffectRemoved_InfoDelegate(Handle);
	if (!Removed)
	{
		Applied.Invalidate();
		EndTask();
		return;
	}
	RemovedHandle = Removed->AddUObject(this, &ThisClass::HandleEffectRemoved);
}

void UAgAbilityTask_ApplyEffect::OnDestroy(bool bInOwnerFinished)
{
	if (Applied.IsValid() && AbilitySystemComponent.IsValid())
	{
		// 지우면서 부른 알림이 Task를 다시 끝내지 않도록 먼저 풉니다.
		if (FOnActiveGameplayEffectRemoved_Info* Removed = AbilitySystemComponent->OnGameplayEffectRemoved_InfoDelegate(Applied))
		{
			Removed->Remove(RemovedHandle);
		}
		if (Params.bRemoveOnEnd)
		{
			AbilitySystemComponent->RemoveActiveGameplayEffect(Applied);
		}
	}
	Super::OnDestroy(bInOwnerFinished);
}

void UAgAbilityTask_ApplyEffect::HandleEffectRemoved(const FGameplayEffectRemovalInfo& RemovalInfo)
{
	Applied.Invalidate();
	EndTask();
}
