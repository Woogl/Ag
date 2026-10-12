// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgEffect_PatternCooldown.h"

#include "AbilitySystemComponent.h"

UAgEffect_PatternCooldown::UAgEffect_PatternCooldown()
{
	// 실제 시간은 Apply가 스펙마다 정합니다. 이 값은 유효한 자리 표시입니다.
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.f));
}

void UAgEffect_PatternCooldown::Apply(UAbilitySystemComponent* ASC, const UObject* Source, float Duration)
{
	if (!ASC || !Source || Duration <= 0.f)
	{
		return;
	}
	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(Source);
	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(StaticClass(), 1.f, Context);
	Spec.Data->SetDuration(Duration, /*bLockDuration*/ true);
	ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
}

bool UAgEffect_PatternCooldown::IsActive(const UAbilitySystemComponent* ASC, const UObject* Source)
{
	if (!ASC || !Source)
	{
		return false;
	}
	FGameplayEffectQuery Query;
	Query.EffectDefinition = StaticClass();
	Query.EffectSource = Source;
	return ASC->GetActiveEffects(Query).Num() > 0;
}
