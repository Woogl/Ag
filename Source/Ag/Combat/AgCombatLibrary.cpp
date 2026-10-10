// Copyright Woogle. All Rights Reserved.

#include "Combat/AgCombatLibrary.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/AgGameplayEffect_Damage.h"
#include "AbilitySystem/AgGameplayEffect_Resource.h"
#include "AbilitySystem/AgGameplayEffect_TimedTag.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Ag.h"
#include "Character/AgBossCharacter.h"
#include "Character/AgCharacterBase.h"
#include "Character/AgPlayerCharacter.h"
#include "Combat/AgTimeSubsystem.h"
#include "Core/AgGameplayTags.h"
#include "Core/AgSettings.h"
#include "Data/AgAttackData.h"
#include "Data/AgCameraData.h"
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

	/** PP can drop unless the character has infinite PP or is groggy (전투 시스템 'PP 감소량 계산식'). */
	bool CanLosePP(const AAgCharacterBase* Character)
	{
		return !Character->GetCharacterData()->Stats.bInfinitePP
			&& !Character->GetAbilitySystemComponent()->HasMatchingGameplayTag(AgGameplayTags::State_Groggy);
	}

	/** 그로기 판정: PP at 0 or below starts groggy, once. */
	bool StartGroggyIfBroken(AAgCharacterBase* Character, AActor* Instigator)
	{
		if (Character->GetAttributeSet()->GetPP() <= 0.f && CanLosePP(Character))
		{
			SendEvent(Character, AgGameplayTags::Event_Groggy, Instigator);
			return true;
		}
		return false;
	}

	/**
	 * 피격 반응: 움찔 plays on top; 넉백 and 다운 start a reaction ability unless super armor or groggy blocks them.
	 * Instigator is what the target is pushed away from: the attacker, or its projectile.
	 */
	void StartHitReaction(AActor* Instigator, AAgCharacterBase* Target, EAgHitReaction Reaction)
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
				SendEvent(Target, Reaction == EAgHitReaction::KnockBack ? AgGameplayTags::Event_Hit_KnockBack : AgGameplayTags::Event_Hit_Down, Instigator);
			}
			break;
		}
		default:
			break;
		}
	}

	/** 가드 반응: 가드 움찔 plays on the guard pose; 넉백 and 다운 become 가드 밀림. */
	void StartGuardReaction(AActor* Instigator, AAgCharacterBase* Target, EAgHitReaction Reaction)
	{
		switch (Reaction)
		{
		case EAgHitReaction::Flinch:
			Target->PlayGuardFlinch();
			break;
		case EAgHitReaction::KnockBack:
		case EAgHitReaction::Down:
			SendEvent(Target, AgGameplayTags::Event_Guard_Pushback, Instigator);
			break;
		default:
			break;
		}
	}

	/** 카메라 '상황별 단계' for a hit that connected. */
	EAgCameraShake GetHitShake(const UAgCameraData& Camera, EAgHitResult Result, const FAgAttackHit& Hit, bool bPlayerAttacked)
	{
		const bool bHeavy = Hit.HitReaction == EAgHitReaction::KnockBack || Hit.HitReaction == EAgHitReaction::Down;
		switch (Result)
		{
		case EAgHitResult::Hit:
			if (bPlayerAttacked)
			{
				return Hit.bFinisher ? Camera.FinisherShake : (bHeavy ? Camera.PlayerHitHeavyShake : Camera.PlayerHitShake);
			}
			return bHeavy ? Camera.PlayerHurtHeavyShake : Camera.PlayerHurtShake;
		case EAgHitResult::Parry:
			return Camera.ParryShake;
		case EAgHitResult::Guard:
			return bHeavy ? Camera.GuardPushbackShake : Camera.GuardShake;
		case EAgHitResult::GuardBreak:
			return Camera.GuardBreakShake;
		default:
			return EAgCameraShake::None;
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

EAgHitResult UAgCombatLibrary::ProcessHit(AAgCharacterBase* Attacker, AAgCharacterBase* Target, const FAgAttackHit& Hit, const FVector& HitLocation, AActor* HitSource)
{
	if (!AreHostile(Attacker, Target) || Target->IsDead())
	{
		return EAgHitResult::None;
	}

	// 공격 방향: a projectile pushes from where it is.
	AActor* Instigator = HitSource ? HitSource : static_cast<AActor*>(Attacker);
	UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
	if (TargetASC->HasMatchingGameplayTag(AgGameplayTags::State_PerfectDodgeWindow))
	{
		// 극한 회피: the dodge ability gives the rewards, once per dodge.
		SendEvent(Target, AgGameplayTags::Event_PerfectDodge, Instigator);
		return EAgHitResult::PerfectDodge;
	}
	if (TargetASC->HasMatchingGameplayTag(AgGameplayTags::State_Invincible))
	{
		return EAgHitResult::None;
	}

	const UAgCombatRules* Rules = GetCombatRules();
	const UAgAttributeSet* SourceStats = Attacker->GetAttributeSet();
	const UAgAttributeSet* TargetStats = Target->GetAttributeSet();
	const UAgPlayerData* DefenderData = Cast<UAgPlayerData>(Target->GetCharacterData());
	const int32 HPDamage = CalculateHPDamage(SourceStats->GetATK(), Hit.DamageMultiplier, TargetStats->GetDEF());

	EAgHitResult Result = EAgHitResult::Hit;
	if (TargetASC->HasMatchingGameplayTag(AgGameplayTags::State_ParryWindow) && Hit.bGuardable && Hit.bParryable && DefenderData)
	{
		// 패리: the defender takes nothing; the attacker loses a share of its MaxPP.
		Result = EAgHitResult::Parry;
		if (CanLosePP(Attacker))
		{
			ApplyStatChange(Target, Attacker, 0.f, -FMath::RoundToFloat(SourceStats->GetMaxPP() * DefenderData->ParryPPRatio));
		}
	}
	else if (TargetASC->HasMatchingGameplayTag(AgGameplayTags::State_Guarding) && Hit.bGuardable && DefenderData)
	{
		// 가드 or 가드 브레이크: reduced HP damage and no PP damage; SP pays for the guard.
		const float GuardSPCost = HPDamage * DefenderData->GuardSPCostRatio;
		const int32 GuardedHPDamage = CalculateHPDamage(SourceStats->GetATK(), Hit.DamageMultiplier, TargetStats->GetDEF(), Rules ? Rules->GuardReduction : 1.f);
		Result = TargetStats->GetSP() >= GuardSPCost ? EAgHitResult::Guard : EAgHitResult::GuardBreak;
		ApplyStatChange(Attacker, Target, -GuardedHPDamage, 0.f);
		ApplyResourceChange(Target, { Result == EAgHitResult::Guard ? -GuardSPCost : -TargetStats->GetSP(), 0.f, 0.f });
	}
	else
	{
		// 일반 피격
		const int32 PPDamage = CanLosePP(Target) ? CalculatePPDamage(SourceStats->GetATK(), Hit.PoiseMultiplier, TargetStats->GetDEF()) : 0;
		ApplyStatChange(Attacker, Target, -HPDamage, -PPDamage);

		// 전투 HUD '대미지 숫자': the HP the boss lost, where the attack touched it. The player's damage shows no number.
		if (const AAgBossCharacter* Boss = Cast<AAgBossCharacter>(Target))
		{
			Boss->ShowDamageNumber(HPDamage, HitLocation, Hit.bLargeDamageNumber);
		}
	}
	UE_LOG(LogAg, Verbose, TEXT("Hit: %s -> %s, result %d, HP now %.0f, PP now %.0f, SP now %.0f, attacker PP now %.0f"),
		*Attacker->GetName(), *Target->GetName(), static_cast<int32>(Result), TargetStats->GetHP(), TargetStats->GetPP(), TargetStats->GetSP(), SourceStats->GetPP());

	// 사망·그로기 판정 comes before the reactions (상태 우선순위). The parry's PP loss is judged on the attacker.
	if (TargetStats->GetHP() <= 0.f)
	{
		SendEvent(Target, AgGameplayTags::Event_Death, Instigator);
	}
	else
	{
		switch (Result)
		{
		case EAgHitResult::Parry:
			SendEvent(Target, AgGameplayTags::Event_Parry, Instigator);
			StartGroggyIfBroken(Attacker, Target);
			break;
		case EAgHitResult::Guard:
			StartGuardReaction(Instigator, Target, Hit.HitReaction);
			break;
		case EAgHitResult::GuardBreak:
			SendEvent(Target, AgGameplayTags::Event_Guard_Break, Instigator);
			break;
		default:
			if (!StartGroggyIfBroken(Target, Instigator))
			{
				StartHitReaction(Instigator, Target, Hit.HitReaction);
			}
			break;
		}
	}

	// 적중 후 처리: effect (the parry has its own), camera shake, hitstop, MP·UP 충전.
	FGameplayCueParameters EffectParameters;
	EffectParameters.Location = HitLocation;
	TargetASC->ExecuteGameplayCue(Result == EAgHitResult::Parry ? AgGameplayTags::GameplayCue_Parry : AgGameplayTags::GameplayCue_Hit, EffectParameters);
	if (const UAgCameraData* Camera = GetCameraData())
	{
		PlayCameraShake(Target, GetHitShake(*Camera, Result, Hit, Attacker->IsA<AAgPlayerCharacter>()));
	}

	// A projectile doesn't stop its shooter; only the character it reached stops.
	UAgTimeSubsystem* TimeSubsystem = UWorld::GetSubsystem<UAgTimeSubsystem>(Target->GetWorld());
	if (TimeSubsystem && HitSource)
	{
		TimeSubsystem->StartHitstop({ Target });
	}
	else if (TimeSubsystem)
	{
		TimeSubsystem->StartHitstop({ Attacker, Target });
	}
	if (TimeSubsystem && Result == EAgHitResult::Parry)
	{
		// 슬로우모션 starts right after the parry's hitstop.
		TimeSubsystem->StartSlowMotion(DefenderData->SlowMotionSpeed, DefenderData->SlowMotionDuration);
	}
	if (Result == EAgHitResult::Hit && (Hit.MPCharge > 0.f || Hit.UPCharge > 0.f))
	{
		ApplyResourceChange(Attacker, { 0.f, Hit.MPCharge, Hit.UPCharge });
	}
	else if (Result == EAgHitResult::Parry)
	{
		ApplyResourceChange(Target, { 0.f, DefenderData->ParryMPCharge, DefenderData->ParryUPCharge });
	}
	return Result;
}

void UAgCombatLibrary::ApplyStatChange(AAgCharacterBase* Source, AAgCharacterBase* Target, float HPChange, float PPChange)
{
	UAbilitySystemComponent* SourceASC = Source->GetAbilitySystemComponent();
	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddInstigator(Source, Source);

	const FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(UAgGameplayEffect_Damage::StaticClass(), 1.f, Context);
	Spec.Data->SetSetByCallerMagnitude(AgGameplayTags::Data_HP, HPChange);
	Spec.Data->SetSetByCallerMagnitude(AgGameplayTags::Data_PP, PPChange);
	SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, Target->GetAbilitySystemComponent());
}

void UAgCombatLibrary::ApplyResourceChange(AAgCharacterBase* Character, const FAgResourceAmounts& Change)
{
	UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();
	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UAgGameplayEffect_Resource::StaticClass(), 1.f, ASC->MakeEffectContext());
	Spec.Data->SetSetByCallerMagnitude(AgGameplayTags::Data_SP, Change.SP);
	Spec.Data->SetSetByCallerMagnitude(AgGameplayTags::Data_MP, Change.MP);
	Spec.Data->SetSetByCallerMagnitude(AgGameplayTags::Data_UP, Change.UP);
	ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
}

FActiveGameplayEffectHandle UAgCombatLibrary::ApplyTimedTag(UAbilitySystemComponent* ASC, const FGameplayTag& Tag, float Duration)
{
	if (!ASC || !Tag.IsValid() || Duration <= 0.f)
	{
		return FActiveGameplayEffectHandle();
	}
	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UAgGameplayEffect_TimedTag::StaticClass(), 1.f, ASC->MakeEffectContext());
	Spec.Data->SetDuration(Duration, /*bLockDuration*/ true);
	Spec.Data->DynamicGrantedTags.AddTag(Tag);
	return ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
}

void UAgCombatLibrary::ApplyRegen(AAgCharacterBase* Character, TSubclassOf<UGameplayEffect> RegenClass, const FGameplayTag& DataTag, float RatePerSecond)
{
	const UAgCombatRules* Rules = GetCombatRules();
	if (!Rules || !RegenClass || RatePerSecond == 0.f)
	{
		return;
	}
	UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();
	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(RegenClass, 1.f, ASC->MakeEffectContext());
	Spec.Data->Period = Rules->RegenTickInterval;
	Spec.Data->SetSetByCallerMagnitude(DataTag, RatePerSecond * Rules->RegenTickInterval);
	ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
}

void UAgCombatLibrary::PlayCameraShake(AAgCharacterBase* Context, EAgCameraShake Shake)
{
	FGameplayTag Cue;
	switch (Shake)
	{
	case EAgCameraShake::Small:
		Cue = AgGameplayTags::GameplayCue_CameraShake_Small;
		break;
	case EAgCameraShake::Medium:
		Cue = AgGameplayTags::GameplayCue_CameraShake_Medium;
		break;
	case EAgCameraShake::Large:
		Cue = AgGameplayTags::GameplayCue_CameraShake_Large;
		break;
	default:
		return;
	}
	if (Context)
	{
		Context->GetAbilitySystemComponent()->ExecuteGameplayCue(Cue, FGameplayCueParameters());
	}
}

const UAgCameraData* UAgCombatLibrary::GetCameraData()
{
	const UAgCameraData* Camera = UAgSettings::Get()->CameraData.LoadSynchronous();
	if (!Camera)
	{
		UE_LOG(LogAg, Error, TEXT("Camera data is not set (Project Settings > Game > Ag)."));
	}
	return Camera;
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
