// Copyright Woogle. All Rights Reserved.
#include "AbilitySystem/AgAbilitySystemComponent.h"

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
