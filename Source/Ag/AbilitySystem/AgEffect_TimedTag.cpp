// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgEffect_TimedTag.h"

#include "AbilitySystemComponent.h"

UAgEffect_TimedTag::UAgEffect_TimedTag()
{
	// 실제 시간은 Apply가 스펙마다 정합니다. 이 값은 유효한 자리 표시입니다.
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.f));
}

FActiveGameplayEffectHandle UAgEffect_TimedTag::Apply(UAbilitySystemComponent* ASC, const FGameplayTag& Tag, float Duration)
{
	if (!ASC || !Tag.IsValid() || Duration <= 0.f)
	{
		return FActiveGameplayEffectHandle();
	}
	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(StaticClass(), 1.f, ASC->MakeEffectContext());
	Spec.Data->SetDuration(Duration, /*bLockDuration*/ true);
	Spec.Data->DynamicGrantedTags.AddTag(Tag);
	return ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
}
