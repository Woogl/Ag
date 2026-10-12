// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgEffect_Damage.h"

#include "AbilitySystem/AgDamageExecution.h"
#include "AbilitySystem/AgHitResultGameplayEffectComponent.h"
#include "AbilitySystemComponent.h"
#include "Character/AgCharacterBase.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgAttackData.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"

UAgEffect_Damage::UAgEffect_Damage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayEffectExecutionDefinition& Execution = Executions.AddDefaulted_GetRef();
	Execution.CalculationClass = UAgDamageExecution::StaticClass();

	UTargetTagRequirementsGameplayEffectComponent* TargetTags = CreateDefaultSubobject<UTargetTagRequirementsGameplayEffectComponent>(TEXT("TargetTagRequirements"));
	TargetTags->ApplicationTagRequirements.IgnoreTags.AddTag(AgGameplayTags::State_Dead);
	GEComponents.Add(TargetTags);
	GEComponents.Add(CreateDefaultSubobject<UAgHitResultGameplayEffectComponent>(TEXT("HitResult")));
}

void UAgEffect_Damage::ApplyAttack(AAgCharacterBase* Attacker, AActor* Causer, AAgCharacterBase* Target, const FAgPlayerAttackHit& Hit, const FVector& HitLocation)
{
	UAbilitySystemComponent* SourceASC = Attacker->GetAbilitySystemComponent();
	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddInstigator(Attacker, Causer);

	const FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(StaticClass(), 1.f, Context);
	SetAttack(*Spec.Data, Hit, HitLocation);
	SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, Target->GetAbilitySystemComponent());
}

void UAgEffect_Damage::SetAttack(FGameplayEffectSpec& Spec, const FAgPlayerAttackHit& Hit, const FVector& HitLocation)
{
	Spec.GetContext().AddOrigin(HitLocation);
	Spec.SetSetByCallerMagnitude(AgGameplayTags::SetByCaller_DamageMultiplier, Hit.DamageMultiplier);
	Spec.SetSetByCallerMagnitude(AgGameplayTags::SetByCaller_PoiseMultiplier, Hit.PoiseMultiplier);
	Spec.SetSetByCallerMagnitude(AgGameplayTags::SetByCaller_MP, Hit.MPCharge);
	Spec.SetSetByCallerMagnitude(AgGameplayTags::SetByCaller_UP, Hit.UPCharge);

	switch (Hit.HitReaction)
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
	if (!Hit.bGuardable)
	{
		Spec.AddDynamicAssetTag(AgGameplayTags::Effect_Damage_Unguardable);
	}
	if (!Hit.bParryable)
	{
		Spec.AddDynamicAssetTag(AgGameplayTags::Effect_Damage_Unparryable);
	}
	if (Hit.bFinisher)
	{
		Spec.AddDynamicAssetTag(AgGameplayTags::Effect_Damage_Finisher);
	}
}
