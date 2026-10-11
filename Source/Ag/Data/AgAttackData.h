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

	/** 전투 HUD '대미지 숫자': 이 타의 숫자를 큰 크기로 표시합니다(스킬·궁극기·처형). */
	UPROPERTY(EditAnywhere, Category = "Attack")
	bool bLargeDamageFloater = false;

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

/** 보스 패턴 하나 (보스 사양 '패턴 목록', '패턴 상세') */
USTRUCT(BlueprintType)
struct FAgBossPattern
{
	GENERATED_BODY()

	/** 패턴 ID. 이 태그로 DA_Boss에서 패턴의 몽타주, 어빌리티, 쿨다운 태그를 찾습니다. */
	UPROPERTY(EditAnywhere, Category = "Pattern", meta = (Categories = "Ability.Boss.Pattern"))
	FGameplayTag Pattern;

	/** 판정 구간마다 하나씩, 몽타주 순서대로 */
	UPROPERTY(EditAnywhere, Category = "Pattern")
	TArray<FAgAttackHit> Hits;

	UPROPERTY(EditAnywhere, Category = "Pattern", meta = (Units = "s", ClampMin = 0))
	float Cooldown = 0.f;

	/** 페이즈: 이 패턴을 쓸 수 있는 페이즈 번호. 비어 있으면 어느 페이즈에서도 쓰지 않습니다. */
	UPROPERTY(EditAnywhere, Category = "Pattern", meta = (ClampMin = 1))
	TArray<int32> Phases;

	/** 거리 구간별 가중치 (근거리). 0이면 그 구간의 후보에서 빠집니다. */
	UPROPERTY(EditAnywhere, Category = "Pattern", meta = (ClampMin = 0))
	float WeightNear = 0.f;

	/** 거리 구간별 가중치 (중거리) */
	UPROPERTY(EditAnywhere, Category = "Pattern", meta = (ClampMin = 0))
	float WeightMid = 0.f;

	/** 거리 구간별 가중치 (원거리) */
	UPROPERTY(EditAnywhere, Category = "Pattern", meta = (ClampMin = 0))
	float WeightFar = 0.f;

	/**
	 * 보스 사양 '이동': 최대 접근 거리. 몽타주의 워핑 구간 동안 모션 자체의 전진에 더해 다가갈 수 있는 거리의 상한입니다.
	 * 워핑 구간이 없는 패턴은 쓰지 않습니다.
	 */
	UPROPERTY(EditAnywhere, Category = "Pattern", meta = (Units = "cm", ClampMin = 0))
	float MaxApproachDistance = 0.f;
};
