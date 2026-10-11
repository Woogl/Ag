// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AgAttackData.generated.h"

/** 전투 시스템 '피격 반응' */
UENUM(BlueprintType)
enum class EAgHitReaction : uint8
{
	None UMETA(DisplayName = "없음"),
	Flinch UMETA(DisplayName = "움찔"),
	KnockBack UMETA(DisplayName = "넉백"),
	KnockDown UMETA(DisplayName = "넉다운"),
	KnockUp UMETA(DisplayName = "넉업"),
};

/** 전투 시스템 '공격 데이터': the values of one attack window (1타), for player and boss attacks alike. */
USTRUCT(BlueprintType)
struct FAgAttackHit
{
	GENERATED_BODY()

	/** 대미지 배율 */
	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = 0))
	float DamageMultiplier = 1.f;

	/** PP 배율 */
	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = 0))
	float PoiseMultiplier = 1.f;

	UPROPERTY(EditAnywhere, Category = "Attack")
	EAgHitReaction HitReaction = EAgHitReaction::Flinch;

	/** 가드 가능 */
	UPROPERTY(EditAnywhere, Category = "Attack")
	bool bGuardable = true;

	/** 패리 가능. An unguardable attack is never parryable. */
	UPROPERTY(EditAnywhere, Category = "Attack", meta = (EditCondition = "bGuardable"))
	bool bParryable = true;
};

/** A player attack's window: the attack data plus what only player attacks have. */
USTRUCT(BlueprintType)
struct FAgPlayerAttackHit : public FAgAttackHit
{
	GENERATED_BODY()

	FAgPlayerAttackHit() = default;

	/** A boss attack's window, with none of the player's charges or display flags. */
	explicit FAgPlayerAttackHit(const FAgAttackHit& Hit) : FAgAttackHit(Hit) {}

	/** 플레이어 사양 'MP 충전': MP the attacker gains when this hit lands. */
	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = 0))
	float MPCharge = 0.f;

	/** 플레이어 사양 'UP 충전': UP the attacker gains when this hit lands. */
	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = 0))
	float UPCharge = 0.f;

	/** 전투 HUD '대미지 숫자': this hit's number shows at the large size (스킬·궁극기·처형). */
	UPROPERTY(EditAnywhere, Category = "Attack")
	bool bLargeDamageNumber = false;

	/** 카메라 '상황별 단계': this hit shakes the camera as a 마지막 일격 (궁극기) instead of an ordinary hit. */
	UPROPERTY(EditAnywhere, Category = "Attack")
	bool bFinisher = false;
};

/** A player attack action (평타 한 타, 회피 반격, 스킬, 궁극기). */
USTRUCT(BlueprintType)
struct FAgPlayerAttack
{
	GENERATED_BODY()

	/** One entry per attack window, in montage order. */
	UPROPERTY(EditAnywhere, Category = "Attack")
	TArray<FAgPlayerAttackHit> Hits;

	/** 타겟팅 범위: a boss this close (capsule centers, horizontal) becomes the target of 공격 방향 and 공격 중 이동. */
	UPROPERTY(EditAnywhere, Category = "Attack", meta = (Units = "cm", ClampMin = 0))
	float TargetingRange = 0.f;
};

/** One boss pattern (보스 사양 '패턴 목록', '거리 구간별 가중치', '패턴 상세'). */
USTRUCT(BlueprintType)
struct FAgBossPattern
{
	GENERATED_BODY()

	/** Pattern ID. The pattern's montage, ability and cooldown tag are found in DA_Boss by this tag. */
	UPROPERTY(EditAnywhere, Category = "Pattern", meta = (Categories = "Ability.Boss.Pattern"))
	FGameplayTag Pattern;

	/** One entry per attack window, in montage order. */
	UPROPERTY(EditAnywhere, Category = "Pattern")
	TArray<FAgAttackHit> Hits;

	UPROPERTY(EditAnywhere, Category = "Pattern", meta = (Units = "s", ClampMin = 0))
	float Cooldown = 0.f;

	/** 페이즈: usable in phase 1. */
	UPROPERTY(EditAnywhere, Category = "Pattern")
	bool bPhase1 = true;

	/** 페이즈: usable in phase 2. */
	UPROPERTY(EditAnywhere, Category = "Pattern")
	bool bPhase2 = true;

	/** 거리 구간별 가중치 (근거리). 0 leaves the pattern out of that band. */
	UPROPERTY(EditAnywhere, Category = "Pattern", meta = (ClampMin = 0))
	float WeightNear = 0.f;

	/** 거리 구간별 가중치 (중거리) */
	UPROPERTY(EditAnywhere, Category = "Pattern", meta = (ClampMin = 0))
	float WeightMid = 0.f;

	/** 거리 구간별 가중치 (원거리) */
	UPROPERTY(EditAnywhere, Category = "Pattern", meta = (ClampMin = 0))
	float WeightFar = 0.f;

	/** 보스 사양 '이동': during the first hit's windup the boss closes in on the stop distance (A1, A5). */
	UPROPERTY(EditAnywhere, Category = "Pattern")
	bool bApproachFirstHit = false;
};
