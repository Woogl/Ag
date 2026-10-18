// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AgAttackData.generated.h"

struct FGameplayEffectSpec;

/** 전투 시스템 '피격 반응' */
UENUM(BlueprintType)
enum class EAgHitReaction : uint8
{
	None UMETA(DisplayName = "없음"),
	Flinch UMETA(DisplayName = "움찔"),
	KnockBack UMETA(DisplayName = "넉백"),
	KnockUp UMETA(DisplayName = "넉업"),
};

/** 전투 시스템 '공격 데이터': 공격 판정 하나(1타)의 값. 플레이어와 보스 공격이 함께 씁니다. */
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

	/** 가드 가능. 불가면 가드와 패리로 막을 수 없습니다. */
	UPROPERTY(EditAnywhere, Category = "Attack")
	bool bGuardable = true;
};

/** 플레이어 공격의 타 하나: 공격 데이터에 플레이어 공격에만 있는 값을 더합니다. */
USTRUCT(BlueprintType)
struct FAgPlayerAttackHit : public FAgAttackHit
{
	GENERATED_BODY()

	FAgPlayerAttackHit() = default;

	/** 보스 공격의 타 하나로 만듭니다. 플레이어 공격에만 있는 값은 기본값입니다. */
	explicit FAgPlayerAttackHit(const FAgAttackHit& Hit) : FAgAttackHit(Hit)
	{
	}

	/** 플레이어 사양 'MP 충전': 이 타가 적중하면 공격자가 얻는 MP */
	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = 0))
	float MPCharge = 0.f;

	/** 플레이어 사양 'UP 충전': 이 타가 적중하면 공격자가 얻는 UP */
	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = 0))
	float UPCharge = 0.f;

	/** 마지막 일격: 처형의 마지막 일격과 궁극기 마무리(카메라 '상황별 단계'). 시간표 Attack 항목의 bFinisher입니다. */
	bool bFinisher = false;

	/** 이 타의 값을 대미지 효과 스펙에 넣습니다: 배율과 충전은 SetByCaller, 반응·가드 불가·마지막 일격은 동적 에셋 태그 */
	void AddToSpec(FGameplayEffectSpec& Spec) const;
};

/** 투사체를 쏘는 행동마다 정하는 값 (전투 시스템 '검기') */
USTRUCT(BlueprintType)
struct FAgProjectileParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Projectile", meta = (Units = "cm/s", ClampMin = 0))
	float Speed = 0.f;

	/** 최대 사거리 */
	UPROPERTY(EditAnywhere, Category = "Projectile", meta = (Units = "cm", ClampMin = 0))
	float Range = 0.f;
};
