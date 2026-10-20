// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgAttributeSet.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Character/AgCharacterBase.h"
#include "Core/AgGameplayTags.h"
#include "GameplayEffectExtension.h"

namespace
{
	/** 받는 쪽이 Instigator를 밀려나는 기준으로 쓰므로 공격자 대신 EffectCauser(공격자 또는 투사체)를 넘깁니다. */
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

	// 즉시·주기 효과(대미지, 비용, 리젠)는 기본값을 바꿉니다. 최대값에서도 리젠이 더해져 기본값이 최대값을 넘으면
	// 넘친 만큼 다음 감소가 묻히므로 범위로 자릅니다.
	ClampToRange(Attribute, NewValue);

	// PP를 줄이는 효과마다 조건을 두지 않도록, PP를 잃을 수 없는 캐릭터의 감소는 여기서 막습니다.
	if (Attribute == GetPPAttribute() && NewValue < PP.GetBaseValue() && !CastChecked<AAgCharacterBase>(GetOwningActor())->CanLosePP())
	{
		NewValue = PP.GetBaseValue();
	}
}

void UAgAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	ClampToRange(Attribute, NewValue);
}

bool UAgAttributeSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	if (!Super::PreGameplayEffectExecute(Data))
	{
		return false;
	}
	if (Data.EvaluatedData.Attribute == GetPPAttribute() && Data.EvaluatedData.ModifierOp == EGameplayModOp::AddBase &&
		Data.EvaluatedData.Magnitude < 0.f)
	{
		// 남은 PP의 소수부를 보존하고, 음수의 .5도 감소량 기준으로 올림하도록 절댓값을 반올림합니다.
		Data.EvaluatedData.Magnitude = -FMath::RoundToFloat(-Data.EvaluatedData.Magnitude);
	}
	return true;
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
