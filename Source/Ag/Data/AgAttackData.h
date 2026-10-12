// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
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
	explicit FAgPlayerAttackHit(const FAgAttackHit& Hit) : FAgAttackHit(Hit) {}

	/** 플레이어 사양 'MP 충전': 이 타가 적중하면 공격자가 얻는 MP */
	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = 0))
	float MPCharge = 0.f;

	/** 플레이어 사양 'UP 충전': 이 타가 적중하면 공격자가 얻는 UP */
	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = 0))
	float UPCharge = 0.f;

	/** 카메라 '상황별 단계': 처형의 마지막 일격과 궁극기 마무리. 데이터가 아니라 그 어빌리티가 켭니다. */
	bool bFinisher = false;
};

/** 플레이어 공격 하나 (평타 한 타, 점프 공격, 회피 반격, 스킬, 처형) */
USTRUCT(BlueprintType)
struct FAgPlayerAttack
{
	GENERATED_BODY()

	/** 타마다 하나씩, 몽타주 순서대로 */
	UPROPERTY(EditAnywhere, Category = "Attack")
	TArray<FAgPlayerAttackHit> Hits;

	/** 타겟팅 범위: 이 거리(캡슐 중심 사이 수평 거리) 안의 보스에게 돌고 다가갑니다(공격 방향, 공격 중 이동). */
	UPROPERTY(EditAnywhere, Category = "Attack", meta = (Units = "cm", ClampMin = 0))
	float TargetingRange = 0.f;

	/** 공격 중 이동: 최대 접근 거리. 각 타의 판정 구간 시작까지 모션 자체의 전진에 더해 다가갈 수 있는 거리의 상한입니다. */
	UPROPERTY(EditAnywhere, Category = "Attack", meta = (Units = "cm", ClampMin = 0))
	float MaxApproachDistance = 0.f;
};

/**
 * 보스 패턴의 전용 값: 한 패턴의 코드만 읽는 값의 부모(구현 계획서 'JSON과 DataAsset' 'JSON 파일'). 전용 값이 있는 패턴
 * 어빌리티가 이 구조체를 상속한 자기 구조체를 어빌리티 옆에 두고, 그 구조체를 정합니다(GetPatternParamsType).
 */
USTRUCT()
struct FAgBossPatternParams
{
	GENERATED_BODY()
};

/** 보스 패턴 하나 (보스 사양 '패턴 목록', '패턴 상세') */
USTRUCT(BlueprintType)
struct FAgBossPattern
{
	GENERATED_BODY()

	/** 패턴 ID. 이 ID로 DA_Boss에서 패턴의 몽타주와 어빌리티를 찾습니다. */
	UPROPERTY(EditAnywhere, Category = "Pattern")
	FName Id;

	/** 타마다 하나씩, 몽타주 순서대로 */
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
	 * 공격 중 이동: 최대 접근 거리. 각 타의 판정 구간 시작까지 모션 자체의 전진에 더해 다가갈 수 있는 거리의 상한입니다.
	 * 몽타주에 워핑 구간이 있는 타만 다가가므로, 워핑 구간이 없는 패턴은 쓰지 않습니다.
	 */
	UPROPERTY(EditAnywhere, Category = "Pattern", meta = (Units = "cm", ClampMin = 0))
	float MaxApproachDistance = 0.f;

	/**
	 * 이 패턴의 코드만 읽는 전용 값. 구조체는 패턴 어빌리티가 정하므로(GetPatternParamsType) JSON에는 값만 적고, 전용 값이
	 * 없는 패턴은 비워 둡니다({}).
	 */
	UPROPERTY(EditAnywhere, Category = "Pattern")
	TInstancedStruct<FAgBossPatternParams> Params;
};
