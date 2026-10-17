// Copyright Woogle. All Rights Reserved.
#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "Core/AgAssetManager.h"
#include "Data/AgEffectData.h"

void UAgAbilitySystemComponent::CancelAbilitiesWithSpecTags(const FGameplayTagContainer& Tags, UGameplayAbility* Ignore)
{
	TArray<FGameplayAbilitySpecHandle> Handles;
	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.IsActive() && Spec.GetPrimaryInstance() != Ignore && Spec.GetDynamicSpecSourceTags().HasAny(Tags))
		{
			Handles.Add(Spec.Handle);
		}
	}
	for (const FGameplayAbilitySpecHandle Handle : Handles)
	{
		CancelAbilityHandle(Handle);
	}
}

void UAgAbilitySystemComponent::ApplyAbilityBlockAndCancelTags(const FGameplayTagContainer& AbilityTags,
    UGameplayAbility* RequestingAbility, bool bEnableBlockTags, const FGameplayTagContainer& BlockTags, bool bExecuteCancelTags,
    const FGameplayTagContainer& CancelTags)
{
	Super::ApplyAbilityBlockAndCancelTags(AbilityTags, RequestingAbility, bEnableBlockTags, BlockTags, false, CancelTags);
	if (bExecuteCancelTags)
	{
		CancelAbilitiesWithSpecTags(CancelTags, RequestingAbility);
	}
}

FGameplayEffectSpecHandle UAgAbilitySystemComponent::MakeEffectSpec(
    EAgEffect Effect, float Level, FGameplayEffectContextHandle Context) const
{
	const UGameplayEffect* Definition = UAgAssetManager::Get().GetEffect(Effect);
	if (!Definition)
	{
		return FGameplayEffectSpecHandle();
	}
	if (!Context.IsValid())
	{
		Context = MakeEffectContext();
	}
	return FGameplayEffectSpecHandle(new FGameplayEffectSpec(Definition, Context, Level));
}
