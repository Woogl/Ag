// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgGameplayEffectContext.h"

UScriptStruct* FAgGameplayEffectContext::GetScriptStruct() const
{
	return StaticStruct();
}

FAgGameplayEffectContext* FAgGameplayEffectContext::Duplicate() const
{
	FAgGameplayEffectContext* Copy = new FAgGameplayEffectContext(*this);
	if (GetHitResult())
	{
		Copy->AddHitResult(*GetHitResult(), true);
	}
	return Copy;
}

TSharedPtr<FAgHitRewardState> FAgGameplayEffectContext::GetHitReward(const FGameplayEffectContextHandle& Context)
{
	const FGameplayEffectContext* Data = Context.Get();
	return Data && Data->GetScriptStruct()->IsChildOf(StaticStruct())
	           ? static_cast<const FAgGameplayEffectContext*>(Data)->HitReward
	           : nullptr;
}
