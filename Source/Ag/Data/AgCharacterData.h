// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AgCharacterData.generated.h"

class UAnimSequenceBase;
class UBlendSpace;
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

	/** 사망 연출 시간. 끝나면 결과 UI를 띄웁니다. (플레이어 사양·보스 사양 '사망') */
	UPROPERTY(EditDefaultsOnly, Category = "Death", meta = (Units = "s", ClampMin = 0))
	float DeathPresentationTime = 0.f;

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
};

/** DA_Boss */
UCLASS(BlueprintType)
class UAgBossData : public UAgCharacterData
{
	GENERATED_BODY()
};
