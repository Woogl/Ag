// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/AgAttackData.h"
#include "Engine/DataAsset.h"
#include "Math/Interval.h"
#include "AgCharacterData.generated.h"

class UAnimMontage;
class UAnimSequenceBase;
class UBlendSpace;
class UGameplayAbility;
class UStreamableRenderAsset;

/** The '스탯' tables of 플레이어 사양 and 보스 사양: starting value and maximum of each stat. */
USTRUCT(BlueprintType)
struct FAgStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = 0))
	float HP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = 0))
	float MaxHP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = 0))
	float SP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = 0))
	float MaxSP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = 0))
	float MP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = 0))
	float MaxMP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = 0))
	float UP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = 0))
	float MaxUP = 0.f;

	/** Ignored when bInfinitePP is set. */
	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = 0, EditCondition = "!bInfinitePP"))
	float PP = 0.f;

	/** Ignored when bInfinitePP is set. */
	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = 0, EditCondition = "!bInfinitePP"))
	float MaxPP = 0.f;

	/** PP 무한: PP never decreases, so the character never becomes groggy. */
	UPROPERTY(EditAnywhere, Category = "Stats")
	bool bInfinitePP = false;

	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = 0))
	float ATK = 0.f;

	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = 0))
	float DEF = 0.f;

	/** 기본 이동 속도 */
	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = 0, Units = "cm/s"))
	float MOV = 0.f;
};

/**
 * Data shared by the player and the boss (DA_Player, DA_Boss).
 * Numbers come from the specs; the 데이터 위치표 in the implementation plan maps each field to its spec item.
 * Numbers and asset references live in separate categories so tuning numbers never touches references.
 */
UCLASS(Abstract, BlueprintType)
class UAgCharacterData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (ShowOnlyInnerProperties))
	FAgStats Stats;

	/** 슈퍼아머 상시 적용 (전투 시스템 '슈퍼아머'): knockback and down never happen. */
	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	bool bAlwaysSuperArmor = false;

	/** 사망 연출 시간. 끝나면 결과 UI를 띄웁니다. (플레이어 사양·보스 사양 '사망') */
	UPROPERTY(EditDefaultsOnly, Category = "Death", meta = (Units = "s", ClampMin = 0))
	float DeathPresentationTime = 0.f;

	/** Start of the weapon's damaging edge, in the weapon mesh's space. Attacks sweep the line from start to end. */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (Units = "cm"))
	FVector WeaponEdgeStart = FVector::ZeroVector;

	/** End (tip) of the weapon's damaging edge, in the weapon mesh's space. */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (Units = "cm"))
	FVector WeaponEdgeEnd = FVector::ZeroVector;

	/** Thickness of the swept edge (sphere radius). The spec gives no number; an implementation value for QA to tune. */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (Units = "cm", ClampMin = 0))
	float WeaponEdgeRadius = 0.f;

	/** Abilities granted at the start: actions, reactions, death. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Abilities")
	TArray<TSubclassOf<UGameplayAbility>> Abilities;

	/** 움찔: additive montage played on top of whatever the character is doing. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> FlinchMontage;

	/** Locomotion while not guarding. Axes: movement direction relative to facing (deg), ground speed (cm/s). */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Animation")
	TObjectPtr<UBlendSpace> MoveBlendSpace;

	/** Locomotion while guarding, same axes as MoveBlendSpace. Leave empty for a character that never guards. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Animation")
	TObjectPtr<UBlendSpace> GuardMoveBlendSpace;

	/** Loop played while falling. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Animation")
	TObjectPtr<UAnimSequenceBase> AirborneAnimation;

	/** Weapon mesh: the katana is a static mesh, the halberd a skeletal mesh. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Weapon", meta = (AllowedClasses = "/Script/Engine.StaticMesh,/Script/Engine.SkeletalMesh"))
	TObjectPtr<UStreamableRenderAsset> WeaponMesh;

	/** Socket on SK_Mannequin the weapon attaches to. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Weapon")
	FName WeaponSocket;
};

/** DA_Player */
UCLASS(BlueprintType)
class UAgPlayerData : public UAgCharacterData
{
	GENERATED_BODY()

public:
	/** 평타: one action per hit, from the first hit. */
	UPROPERTY(EditDefaultsOnly, Category = "Basic Attack")
	TArray<FAgPlayerAttack> BasicAttack;

	/** 공격 중 이동: the gap the player keeps from the target (capsule centers, horizontal). */
	UPROPERTY(EditDefaultsOnly, Category = "Attack Movement", meta = (Units = "cm", ClampMin = 0))
	float AttackStopDistance = 0.f;

	/** 평타 montages, in the same order as BasicAttack. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TArray<TObjectPtr<UAnimMontage>> BasicAttackMontages;

	/** 넉백 */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> KnockBackMontage;

	/** 다운: falling down. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> DownMontage;

	/** 다운: getting up, blended in after DownMontage (the poses differ slightly). */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> GetUpMontage;
};

/** DA_Boss */
UCLASS(BlueprintType)
class UAgBossData : public UAgCharacterData
{
	GENERATED_BODY()

public:
	const FAgBossPattern* FindPattern(const FGameplayTag& Pattern) const;
	UAnimMontage* FindPatternMontage(const FGameplayTag& Pattern) const;

	/** 패턴 목록 */
	UPROPERTY(EditDefaultsOnly, Category = "Patterns")
	TArray<FAgBossPattern> Patterns;

	/** 모션 재생 속도: pattern motions play at this rate in phase 1. */
	UPROPERTY(EditDefaultsOnly, Category = "Pattern Rules", meta = (ClampMin = 0.01))
	float PatternPlayRate = 1.f;

	/** 이동: 정지 간격 (capsule centers, horizontal). */
	UPROPERTY(EditDefaultsOnly, Category = "Pattern Rules", meta = (Units = "cm", ClampMin = 0))
	float StopDistance = 0.f;

	/** 이동: 최대 접근 거리 of the first-hit approach (A1, A5). */
	UPROPERTY(EditDefaultsOnly, Category = "Pattern Rules", meta = (Units = "cm", ClampMin = 0))
	float FirstHitApproachDistance = 0.f;

	/** 회전: turning speed toward the player. */
	UPROPERTY(EditDefaultsOnly, Category = "Pattern Rules", meta = (Units = "deg/s", ClampMin = 0))
	float RotationSpeed = 0.f;

	/** 회전: the boss stops turning this long before each attack window starts, until it ends. */
	UPROPERTY(EditDefaultsOnly, Category = "Pattern Rules", meta = (Units = "s", ClampMin = 0))
	float RotationStopLead = 0.f;

	/** 거리 구간: 근거리 upper bound (capsule centers, horizontal). */
	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (Units = "cm", ClampMin = 0))
	float NearDistance = 0.f;

	/** 거리 구간: 중거리 upper bound. Farther is 원거리. */
	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (Units = "cm", ClampMin = 0))
	float FarDistance = 0.f;

	/** 패턴 선택: with no candidate, the boss walks to the player and retries this often. */
	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (Units = "s", ClampMin = 0.01))
	float RetryInterval = 0.5f;

	/** 패턴 후딜레이 대기 시간 after a pattern in phase 1 (random in range). */
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	FFloatInterval PatternRecoveryWaitPhase1 = FFloatInterval(0.f, 0.f);

	/** 패턴 후딜레이 대기 시간 after a pattern in phase 2 (random in range). */
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	FFloatInterval PatternRecoveryWaitPhase2 = FFloatInterval(0.f, 0.f);

	/** 패턴 후딜레이 대기 시간 after the level start, the phase transition, groggy and being executed. */
	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (Units = "s", ClampMin = 0))
	float FixedRecoveryWait = 0.f;

	/** Pattern montages, by pattern ID. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages", meta = (Categories = "Ability.Boss.Pattern"))
	TMap<FGameplayTag, TObjectPtr<UAnimMontage>> PatternMontages;
};
