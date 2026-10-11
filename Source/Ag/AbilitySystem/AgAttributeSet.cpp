// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgAttributeSet.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Character/AgCharacterBase.h"
#include "Core/AgGameplayTags.h"
#include "GameplayEffectExtension.h"

namespace
{
	/**
	 * 사망·그로기 이벤트를 보냅니다. 밀려나는 기준(공격자 또는 투사체)을 Instigator로, 공격 쪽 태그(공격자의 태그,
	 * 처형 같은 어빌리티의 태그)를 InstigatorTags로 넘깁니다.
	 */
	void SendJudgmentEvent(AActor* Character, const FGameplayTag& EventTag, const FGameplayEffectSpec& Spec)
	{
		FGameplayEventData Payload;
		Payload.EventTag = EventTag;
		Payload.Target = Character;
		Payload.Instigator = Spec.GetContext().GetEffectCauser();
		Payload.InstigatorTags = *Spec.CapturedSourceTags.GetAggregatedTags();
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Character, EventTag, Payload);
	}
}

void UAgAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	// Instant and periodic effects (damage, costs, regen) change the base value; keep it in range so a regen tick at
	// the maximum doesn't pile up above it and swallow the next decrease.
	ClampToRange(Attribute, NewValue);
}

void UAgAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	ClampToRange(Attribute, NewValue);
}

void UAgAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	const AAgCharacterBase* Character = CastChecked<AAgCharacterBase>(GetOwningActor());
	if (Data.EvaluatedData.Attribute == GetHPAttribute() && GetHP() <= 0.f)
	{
		SendJudgmentEvent(GetOwningActor(), AgGameplayTags::Event_Death, Data.EffectSpec);
	}
	else if (Data.EvaluatedData.Attribute == GetPPAttribute() && GetPP() <= 0.f && Character->CanLosePP())
	{
		SendJudgmentEvent(GetOwningActor(), AgGameplayTags::Event_Groggy, Data.EffectSpec);
	}
}

void UAgAttributeSet::ClampToRange(const FGameplayAttribute& Attribute, float& Value) const
{
	if (Attribute == GetHPAttribute())
	{
		Value = FMath::Clamp(Value, 0.f, GetMaxHP());
	}
	else if (Attribute == GetSPAttribute())
	{
		Value = FMath::Clamp(Value, 0.f, GetMaxSP());
	}
	else if (Attribute == GetMPAttribute())
	{
		Value = FMath::Clamp(Value, 0.f, GetMaxMP());
	}
	else if (Attribute == GetUPAttribute())
	{
		Value = FMath::Clamp(Value, 0.f, GetMaxUP());
	}
	else if (Attribute == GetPPAttribute())
	{
		Value = FMath::Clamp(Value, 0.f, GetMaxPP());
	}
	else
	{
		Value = FMath::Max(Value, 0.f);
	}
}
