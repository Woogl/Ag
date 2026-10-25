// Copyright Woogle. All Rights Reserved.

#include "Data/AgAttackData.h"
#include "Core/AgGameplayTags.h"
#include "GameplayEffect.h"

void FAgDamage::AddToSpec(FGameplayEffectSpec& Spec) const
{
	Spec.SetSetByCallerMagnitude(AgGameplayTags::SetByCaller_HPMultiplier, HPMultiplier);
	Spec.SetSetByCallerMagnitude(AgGameplayTags::SetByCaller_PPMultiplier, PPMultiplier);
	switch (HitReaction)
	{
		case EAgHitReaction::Flinch:
			Spec.AddDynamicAssetTag(AgGameplayTags::Effect_Damage_Reaction_Flinch);
			break;
		case EAgHitReaction::KnockUp:
			Spec.AddDynamicAssetTag(AgGameplayTags::Effect_Damage_Reaction_KnockUp);
			break;
		case EAgHitReaction::KnockDown:
			Spec.AddDynamicAssetTag(AgGameplayTags::Effect_Damage_Reaction_KnockDown);
			break;
		default:
			break;
	}
	if (!bGuardable)
	{
		Spec.AddDynamicAssetTag(AgGameplayTags::Effect_Damage_Unblockable);
	}
	if (bFinisher)
	{
		Spec.AddDynamicAssetTag(AgGameplayTags::Effect_Damage_Finisher);
	}
	if (!bAttackerHitstop)
	{
		Spec.AddDynamicAssetTag(AgGameplayTags::Effect_Damage_NoAttackerHitstop);
	}
}
