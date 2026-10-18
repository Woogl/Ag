// Copyright Woogle. All Rights Reserved.

#include "Data/AgAttackData.h"
#include "Core/AgGameplayTags.h"
#include "GameplayEffect.h"

void FAgPlayerAttackHit::AddToSpec(FGameplayEffectSpec& Spec) const
{
	Spec.SetSetByCallerMagnitude(AgGameplayTags::SetByCaller_DamageMultiplier, DamageMultiplier);
	Spec.SetSetByCallerMagnitude(AgGameplayTags::SetByCaller_PoiseMultiplier, PoiseMultiplier);
	Spec.SetSetByCallerMagnitude(AgGameplayTags::SetByCaller_MP, MPCharge);
	Spec.SetSetByCallerMagnitude(AgGameplayTags::SetByCaller_UP, UPCharge);
	switch (HitReaction)
	{
		case EAgHitReaction::Flinch:
			Spec.AddDynamicAssetTag(AgGameplayTags::Effect_Damage_Reaction_Flinch);
			break;
		case EAgHitReaction::KnockUp:
			Spec.AddDynamicAssetTag(AgGameplayTags::Effect_Damage_Reaction_KnockUp);
			break;
		case EAgHitReaction::KnockBack:
			Spec.AddDynamicAssetTag(AgGameplayTags::Effect_Damage_Reaction_KnockBack);
			break;
		default:
			break;
	}
	if (!bGuardable)
	{
		Spec.AddDynamicAssetTag(AgGameplayTags::Effect_Damage_Unguardable);
	}
	if (bFinisher)
	{
		Spec.AddDynamicAssetTag(AgGameplayTags::Effect_Damage_Finisher);
	}
}
