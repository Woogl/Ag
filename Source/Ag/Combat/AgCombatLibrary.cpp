// Copyright Woogle. All Rights Reserved.

#include "Combat/AgCombatLibrary.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/AgGameplayEffect_Damage.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Ag.h"
#include "Character/AgCharacterBase.h"
#include "Character/AgPlayerCharacter.h"
#include "Combat/AgTimeSubsystem.h"
#include "Core/AgGameplayTags.h"
#include "Core/AgSettings.h"
#include "Data/AgAttackData.h"
#include "Data/AgCharacterData.h"
#include "Data/AgCombatRules.h"

namespace
{
	void SendEvent(AAgCharacterBase* Target, const FGameplayTag& EventTag, AActor* Instigator)
	{
		FGameplayEventData Payload;
		Payload.EventTag = EventTag;
		Payload.Instigator = Instigator;
		Payload.Target = Target;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Target, EventTag, Payload);
	}

	void ApplyDamage(AAgCharacterBase* Attacker, AAgCharacterBase* Target, int32 HPDamage, int32 PPDamage)
	{
		UAbilitySystemComponent* SourceASC = Attacker->GetAbilitySystemComponent();
		FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
		Context.AddInstigator(Attacker, Attacker);

		const FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(UAgGameplayEffect_Damage::StaticClass(), 1.f, Context);
		Spec.Data->SetSetByCallerMagnitude(AgGameplayTags::Data_HP, -HPDamage);
		Spec.Data->SetSetByCallerMagnitude(AgGameplayTags::Data_PP, -PPDamage);
		SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, Target->GetAbilitySystemComponent());
	}

	/** 피격 반응: 움찔 plays on top; 넉백 and 다운 start a reaction ability unless super armor or groggy blocks them. */
	void StartHitReaction(AAgCharacterBase* Attacker, AAgCharacterBase* Target, EAgHitReaction Reaction)
	{
		const UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
		switch (Reaction)
		{
		case EAgHitReaction::Flinch:
			// 움찔 doesn't change a knockback or down already playing.
			if (!TargetASC->HasMatchingGameplayTag(AgGameplayTags::State_HitReaction))
			{
				Target->PlayFlinch();
			}
			break;
		case EAgHitReaction::KnockBack:
		case EAgHitReaction::Down:
		{
			FGameplayTagContainer Blockers;
			Blockers.AddTag(AgGameplayTags::State_SuperArmor);
			Blockers.AddTag(AgGameplayTags::State_Groggy);
			if (!TargetASC->HasAnyMatchingGameplayTags(Blockers))
			{
				SendEvent(Target, Reaction == EAgHitReaction::KnockBack ? AgGameplayTags::Event_Hit_KnockBack : AgGameplayTags::Event_Hit_Down, Attacker);
			}
			break;
		}
		default:
			break;
		}
	}
}

int32 UAgCombatLibrary::CalculateHPDamage(float SourceATK, float DamageMultiplier, float TargetDEF, float GuardReduction)
{
	return FMath::RoundToInt(SourceATK * DamageMultiplier * (100.f / (100.f + TargetDEF)) * GuardReduction);
}

int32 UAgCombatLibrary::CalculatePPDamage(float SourceATK, float PoiseMultiplier, float TargetDEF)
{
	return FMath::RoundToInt(SourceATK * PoiseMultiplier * (100.f / (100.f + TargetDEF)));
}

bool UAgCombatLibrary::AreHostile(const AAgCharacterBase* A, const AAgCharacterBase* B)
{
	return A && B && A->IsA<AAgPlayerCharacter>() != B->IsA<AAgPlayerCharacter>();
}

FVector UAgCombatLibrary::GetAttackDirection(const AActor* Source, const AActor* Target)
{
	if (!Source || !Target)
	{
		return FVector::ZeroVector;
	}
	return (Target->GetActorLocation() - Source->GetActorLocation()).GetSafeNormal2D();
}

float UAgCombatLibrary::GetHorizontalDistance(const AActor* A, const AActor* B)
{
	return (A && B) ? FVector::Dist2D(A->GetActorLocation(), B->GetActorLocation()) : TNumericLimits<float>::Max();
}

EAgHitResult UAgCombatLibrary::ProcessHit(AAgCharacterBase* Attacker, AAgCharacterBase* Target, const FAgAttackHit& Hit, const FVector& HitLocation)
{
	if (!AreHostile(Attacker, Target) || Target->IsDead())
	{
		return EAgHitResult::None;
	}

	UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
	if (TargetASC->HasMatchingGameplayTag(AgGameplayTags::State_Invincible))
	{
		return EAgHitResult::None;
	}

	// 일반 피격
	const UAgAttributeSet* SourceStats = Attacker->GetAttributeSet();
	const UAgAttributeSet* TargetStats = Target->GetAttributeSet();
	const int32 HPDamage = CalculateHPDamage(SourceStats->GetATK(), Hit.DamageMultiplier, TargetStats->GetDEF());
	const bool bLosesPP = !Target->GetCharacterData()->Stats.bInfinitePP && !TargetASC->HasMatchingGameplayTag(AgGameplayTags::State_Groggy);
	const int32 PPDamage = bLosesPP ? CalculatePPDamage(SourceStats->GetATK(), Hit.PoiseMultiplier, TargetStats->GetDEF()) : 0;
	ApplyDamage(Attacker, Target, HPDamage, PPDamage);
	UE_LOG(LogAg, Verbose, TEXT("Hit: %s -> %s, HP -%d (now %.0f), PP -%d"), *Attacker->GetName(), *Target->GetName(), HPDamage, TargetStats->GetHP(), PPDamage);

	// 사망·그로기 판정 comes before the hit reaction (상태 우선순위).
	if (TargetStats->GetHP() <= 0.f)
	{
		SendEvent(Target, AgGameplayTags::Event_Death, Attacker);
	}
	else
	{
		StartHitReaction(Attacker, Target, Hit.HitReaction);
	}

	// 적중 후 처리
	if (UAgTimeSubsystem* TimeSubsystem = UWorld::GetSubsystem<UAgTimeSubsystem>(Target->GetWorld()))
	{
		TimeSubsystem->StartHitstop({ Attacker, Target });
	}
	return EAgHitResult::Hit;
}

const UAgCombatRules* UAgCombatLibrary::GetCombatRules()
{
	const UAgCombatRules* Rules = UAgSettings::Get()->CombatRules.LoadSynchronous();
	if (!Rules)
	{
		UE_LOG(LogAg, Error, TEXT("Combat rules are not set (Project Settings > Game > Ag)."));
	}
	return Rules;
}
