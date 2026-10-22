// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgRegenDelayGameplayEffectComponent.h"

#include "AbilitySystemComponent.h"
#include "Core/AgAssetManager.h"
#include "GameplayEffect.h"

bool UAgRegenDelayGameplayEffectComponent::OnActiveGameplayEffectAdded(FActiveGameplayEffectsContainer& GEContainer, FActiveGameplayEffect& ActiveGE) const
{
	UAbilitySystemComponent* ASC = GEContainer.Owner;
	FActiveGameplayEffectEvents* EventSet = ASC ? ASC->GetActiveEffectEventSet(ActiveGE.Handle) : nullptr;
	if (!EventSet)
	{
		return true;
	}

	TArray<FGameplayAttribute> Attributes;
	for (const FGameplayModifierInfo& Modifier : ActiveGE.Spec.Def->Modifiers)
	{
		Attributes.AddUnique(Modifier.Attribute);
	}
	TArray<TTuple<FGameplayAttribute, FDelegateHandle>> Bindings;
	for (const FGameplayAttribute& Attribute : Attributes)
	{
		Bindings.Emplace(Attribute, ASC->GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject(this, &ThisClass::HandleAttributeChanged, ASC));
	}
	EventSet->OnEffectRemoved.AddUObject(this, &ThisClass::HandleEffectRemoved, ASC, MoveTemp(Bindings));
	return true;
}

void UAgRegenDelayGameplayEffectComponent::HandleAttributeChanged(const FOnAttributeChangeData& Data, UAbilitySystemComponent* ASC) const
{
	if (Data.NewValue >= Data.OldValue)
	{
		return;
	}
	const UGameplayEffect* DelayEffect = UAgAssetManager::Get().FindEffect(Delay.Effect);
	if (!DelayEffect)
	{
		return;
	}
	ASC->RemoveActiveEffectsWithGrantedTags(DelayEffect->GetGrantedTags());
	ASC->ApplyGameplayEffectToSelf(DelayEffect, 1.f, ASC->MakeEffectContext());
}

void UAgRegenDelayGameplayEffectComponent::HandleEffectRemoved(const FGameplayEffectRemovalInfo& RemovalInfo, UAbilitySystemComponent* ASC, TArray<TTuple<FGameplayAttribute, FDelegateHandle>> Bindings) const
{
	for (const TTuple<FGameplayAttribute, FDelegateHandle>& Binding : Bindings)
	{
		ASC->GetGameplayAttributeValueChangeDelegate(Binding.Key).Remove(Binding.Value);
	}
}
