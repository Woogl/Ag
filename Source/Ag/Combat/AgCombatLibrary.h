// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AgCombatLibrary.generated.h"

class AAgCharacterBase;
class UAgCombatRules;
struct FAgAttackHit;

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
	 * Runs 피격 처리 순서 for one attack window that touched Target for the first time.
	 * Applies HP and PP changes, starts the death or hit reaction, and the after-hit effects (hitstop).
	 */
	static EAgHitResult ProcessHit(AAgCharacterBase* Attacker, AAgCharacterBase* Target, const FAgAttackHit& Hit, const FVector& HitLocation);

	/** DA_CombatRules, or null with an error when the project setting is empty. */
	static const UAgCombatRules* GetCombatRules();
};
