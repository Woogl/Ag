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

TSharedPtr<FAgHitState> FAgGameplayEffectContext::GetHitState(const FGameplayEffectContextHandle& Context)
{
	const FGameplayEffectContext* Data = Context.Get();
	return Data && Data->GetScriptStruct()->IsChildOf(StaticStruct())
	           ? static_cast<const FAgGameplayEffectContext*>(Data)->HitState
	           : nullptr;
}
