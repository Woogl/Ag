// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AgCombatLibrary.generated.h"

class AAgCharacterBase;
class UAbilitySystemComponent;
class UAgCombatRules;
class UGameplayEffect;
struct FActiveGameplayEffectHandle;
struct FAgAttackHit;
struct FGameplayTag;

/** Amounts of the player resources; positive adds, negative spends. */
struct FAgResourceAmounts
{
	float SP = 0.f;
	float MP = 0.f;
	float UP = 0.f;
};

/** Result of 전투 시스템 '피격 처리 순서'. */
UENUM()
enum class EAgHitResult : uint8
{
	None,
	PerfectDodge,
	Parry,
	Guard,
	GuardBreak,
	Hit,
};

/** Combat rules of 전투 시스템: damage formulas and the hit processing order. */
UCLASS()
class UAgCombatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 'HP 감소량 계산식', times the guard reduction when guarded, rounded to an integer. */
	static int32 CalculateHPDamage(float SourceATK, float DamageMultiplier, float TargetDEF, float GuardReduction = 1.f);

	/** 'PP 감소량 계산식', rounded to an integer. */
	static int32 CalculatePPDamage(float SourceATK, float PoiseMultiplier, float TargetDEF);

	/** Attacks only hit the other side (player <-> boss). */
	static bool AreHostile(const AAgCharacterBase* A, const AAgCharacterBase* B);

	/** 공격 방향: horizontal unit vector from the source's capsule center to the target's. */
	static FVector GetAttackDirection(const AActor* Source, const AActor* Target);

	/** Horizontal distance between two actors' capsule centers (대상과의 간격, 거리 구간). */
	static float GetHorizontalDistance(const AActor* A, const AActor* B);

	/**
	 * Runs 피격 처리 순서 for one attack window that touched Target for the first time:
	 * 극한 회피, 무적, 패리, 가드 or 가드 브레이크, 일반 피격, then 사망·그로기 판정 and 적중 후 처리
	 * (hitstop, MP·UP 충전). Starts the reaction abilities through gameplay events.
	 */
	static EAgHitResult ProcessHit(AAgCharacterBase* Attacker, AAgCharacterBase* Target, const FAgAttackHit& Hit, const FVector& HitLocation);

	/** Applies an instant HP and PP change from Source to Target (negative lowers). */
	static void ApplyStatChange(AAgCharacterBase* Source, AAgCharacterBase* Target, float HPChange, float PPChange);

	/** Applies an instant SP, MP and UP change to Character. */
	static void ApplyResourceChange(AAgCharacterBase* Character, const FAgResourceAmounts& Change);

	/** Gives ASC's owner Tag for Duration seconds of game time (cooldowns, regen delays, the parry window). */
	static FActiveGameplayEffectHandle ApplyTimedTag(UAbilitySystemComponent* ASC, const FGameplayTag& Tag, float Duration);

	/** Starts a regen effect that adds RatePerSecond, in steps of DA_CombatRules' regen tick interval. */
	static void ApplyRegen(AAgCharacterBase* Character, TSubclassOf<UGameplayEffect> RegenClass, const FGameplayTag& DataTag, float RatePerSecond);

	/** DA_CombatRules, or null with an error when the project setting is empty. */
	static const UAgCombatRules* GetCombatRules();
};
