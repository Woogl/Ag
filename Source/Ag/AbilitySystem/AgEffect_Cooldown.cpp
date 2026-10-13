// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgEffect_Cooldown.h"

#include "AbilitySystemComponent.h"

namespace
{
	FGameplayEffectQuery MakeCooldownQuery(const UObject* Source)
	{
		FGameplayEffectQuery Query;
		Query.EffectDefinition = UAgEffect_Cooldown::StaticClass();
		Query.EffectSource = Source;
		return Query;
	}
}

UAgEffect_Cooldown::UAgEffect_Cooldown()
{
	// 실제 시간은 Apply가 스펙마다 정합니다. 이 값은 유효한 자리 표시입니다.
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.f));
}

void UAgEffect_Cooldown::Apply(UAbilitySystemComponent* ASC, const UObject* Source, float Duration)
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

bool UAgEffect_Cooldown::IsActive(const UAbilitySystemComponent* ASC, const UObject* Source)
{
	return ASC && Source && ASC->GetActiveEffects(MakeCooldownQuery(Source)).Num() > 0;
}

void UAgEffect_Cooldown::GetTimeRemaining(const UAbilitySystemComponent* ASC, const UObject* Source, float& OutRemaining, float& OutDuration)
{
	OutRemaining = 0.f;
	OutDuration = 0.f;
	if (!ASC || !Source)
	{
		return;
	}
	for (const TPair<float, float>& Entry : ASC->GetActiveEffectsTimeRemainingAndDuration(MakeCooldownQuery(Source)))
	{
		if (Entry.Key > OutRemaining)
		{
			OutRemaining = Entry.Key;
			OutDuration = Entry.Value;
		}
	}
}
