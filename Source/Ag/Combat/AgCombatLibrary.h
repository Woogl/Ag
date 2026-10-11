// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AgCombatLibrary.generated.h"

class AAgCharacterBase;
class UAbilitySystemComponent;
class UGameplayEffect;
struct FActiveGameplayEffectHandle;
struct FAgPlayerAttackHit;
struct FGameplayTag;
enum class EAgCameraShake : uint8;

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
	 * 판정 구간이 Target에 처음 닿았을 때 '피격 처리 순서'를 실행합니다: 극한 회피, 무적, 패리, 가드 또는 가드 브레이크, 일반 피격,
	 * 그다음 사망·그로기 판정과 적중 후 처리(이펙트, 카메라 셰이크, 히트스톱, MP·UP 충전). 반응 어빌리티는 게임플레이 이벤트로 시작합니다.
	 * HitSource는 공격을 실어 나른 투사체입니다(없으면 null). 대상은 투사체에서 밀려나고, 히트스톱은 대상만 멈춥니다.
	 * 보스 공격의 Hit는 플레이어 전용 필드를 기본값으로 둡니다.
	 */
	static EAgHitResult ProcessHit(AAgCharacterBase* Attacker, AAgCharacterBase* Target, const FAgPlayerAttackHit& Hit, const FVector& HitLocation, AActor* HitSource = nullptr);

	/** Plays a camera shake level through its GameplayCue (카메라 '카메라 셰이크'). Context is any character. */
	static void PlayCameraShake(AAgCharacterBase* Context, EAgCameraShake Shake);

	/** Applies an instant HP and PP change from Source to Target (negative lowers). */
	static void ApplyStatChange(AAgCharacterBase* Source, AAgCharacterBase* Target, float HPChange, float PPChange);

	/** Applies an instant SP, MP and UP change to Character. */
	static void ApplyResourceChange(AAgCharacterBase* Character, const FAgResourceAmounts& Change);

	/** Gives ASC's owner Tag for Duration seconds of game time (cooldowns, regen delays, the parry window, the 회피 반격 기회). */
	static FActiveGameplayEffectHandle ApplyTimedTag(UAbilitySystemComponent* ASC, const FGameplayTag& Tag, float Duration);

	/**
	 * Starts a periodic effect that adds RatePerSecond (negative spends), in steps of CombatRules.json's regen tick
	 * interval: regen, and the sprint's SP cost. Replaces a running effect of the same class.
	 */
	static void ApplyRegen(AAgCharacterBase* Character, TSubclassOf<UGameplayEffect> RegenClass, const FGameplayTag& DataTag, float RatePerSecond);
};
