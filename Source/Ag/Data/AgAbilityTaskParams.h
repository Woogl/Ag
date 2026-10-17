// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/AgAttackData.h"
#include "Engine/EngineTypes.h"
#include "AgAbilityTaskParams.generated.h"

class ULevelSequence;

UENUM()
enum class EAgResource : uint8
{
	SP,
	MP,
	UP
};

USTRUCT()
struct FAgResourceCostParams
{
	GENERATED_BODY()

	UPROPERTY()
	EAgResource Resource = EAgResource::SP;

	UPROPERTY()
	float Amount = 0.f;
};

USTRUCT()
struct FAgPlayMontageAndWaitParams
{
	GENERATED_BODY()

	UPROPERTY()
	FName MontageId;

	UPROPERTY()
	float RootMotionDistance = 0.f;
	bool bHasRootMotionDistance = false;
};

USTRUCT()
struct FAgDrainResourceParams
{
	GENERATED_BODY()

	UPROPERTY()
	EAgResource Resource = EAgResource::SP;

	UPROPERTY()
	float AmountPerSecond = 0.f;
};

USTRUCT()
struct FAgTimedTagParams
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag Tag;

	UPROPERTY()
	float Duration = 0.f;
};

USTRUCT()
struct FAgGrantResourcesParams
{
	GENERATED_BODY()

	UPROPERTY()
	float SP = 0.f;

	UPROPERTY()
	float MP = 0.f;

	UPROPERTY()
	float UP = 0.f;
};

USTRUCT()
struct FAgSlowMotionParams
{
	GENERATED_BODY()

	UPROPERTY()
	float Speed = 0.f;

	UPROPERTY()
	float RealDuration = 0.f;
};

USTRUCT()
struct FAgLaunchVerticalParams
{
	GENERATED_BODY()

	UPROPERTY()
	float VelocityZ = 0.f;
};

USTRUCT()
struct FAgLaunchToTargetParams
{
	GENERATED_BODY()

	UPROPERTY()
	float AirTime = 0.f;

	UPROPERTY()
	float TargetOffset = 0.f;

	UPROPERTY()
	float MaxDistance = 0.f;
};

USTRUCT()
struct FAgTargetQueryParams
{
	GENERATED_BODY()

	UPROPERTY()
	float Range = 0.f;
};

USTRUCT()
struct FAgMotionWarpingParams
{
	GENERATED_BODY()

	UPROPERTY()
	float TargetingRange = 0.f;

	UPROPERTY()
	float MaxWarpDistance = 0.f;
	bool bHasTargetingRange = false;
	bool bHasMaxWarpDistance = false;
};

USTRUCT()
struct FAgAttackParams
{
	GENERATED_BODY()

	UPROPERTY()
	float DamageMultiplier = 0.f;

	UPROPERTY()
	float PoiseMultiplier = 0.f;

	UPROPERTY()
	EAgHitReaction HitReaction = EAgHitReaction::None;

	UPROPERTY()
	bool bGuardable = false;

	UPROPERTY()
	float MPCharge = 0.f;

	UPROPERTY()
	float UPCharge = 0.f;
	FAgPlayerAttackHit MakeHit(bool bFinisher = false) const;
};

USTRUCT()
struct FAgDashParams
{
	GENERATED_BODY()

	UPROPERTY()
	float Speed = 0.f;

	UPROPERTY()
	float HomingTurnRate = 0.f;

	UPROPERTY()
	float Overshoot = 0.f;

	UPROPERTY()
	float MaxDistance = 0.f;
};

USTRUCT()
struct FAgHoldTagParams
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag Tag;
};

USTRUCT()
struct FAgCollisionResponseParams
{
	GENERATED_BODY()

	UPROPERTY()
	TEnumAsByte<ECollisionChannel> Channel = ECC_MAX;

	UPROPERTY()
	TEnumAsByte<ECollisionResponse> Response = ECR_MAX;
};

USTRUCT()
struct FAgPlayCutsceneParams
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<ULevelSequence> Sequence;
};

USTRUCT()
struct FAgSpawnProjectileParams
{
	GENERATED_BODY()

	UPROPERTY()
	FName ProjectileId;

	UPROPERTY()
	FName AttackParamsId;
};

/** 설정 유무는 값과 별개로 유지합니다. 맵은 검증된 비어 있지 않은 블록만 존재합니다. */
USTRUCT()
struct FAgAbilityTasks
{
	GENERATED_BODY()

	UPROPERTY()
	FAgPlayMontageAndWaitParams PlayMontageAndWait;

	UPROPERTY()
	FAgDrainResourceParams DrainResource;

	UPROPERTY()
	FAgLaunchVerticalParams LaunchVertical;

	UPROPERTY()
	FAgLaunchToTargetParams LaunchToTarget;

	UPROPERTY()
	FAgDashParams Dash;

	UPROPERTY()
	TMap<FName, FAgTimedTagParams> TimedTag;

	UPROPERTY()
	TMap<FName, FAgGrantResourcesParams> GrantResources;

	UPROPERTY()
	TMap<FName, FAgSlowMotionParams> SlowMotion;

	UPROPERTY()
	TMap<FName, FAgTargetQueryParams> TargetQuery;

	UPROPERTY()
	TMap<FName, FAgMotionWarpingParams> MotionWarping;

	UPROPERTY()
	TMap<FName, FAgAttackParams> Attack;

	UPROPERTY()
	TMap<FName, FAgHoldTagParams> HoldTag;

	UPROPERTY()
	TMap<FName, FAgCollisionResponseParams> SetCollisionResponseToChannel;

	UPROPERTY()
	TMap<FName, FAgPlayCutsceneParams> PlayCutscene;

	UPROPERTY()
	TMap<FName, FAgSpawnProjectileParams> SpawnProjectile;
	bool bHasPlayMontageAndWait = false;
	bool bHasDrainResource = false;
	bool bHasLaunchVertical = false;
	bool bHasLaunchToTarget = false;
	bool bHasDash = false;
	bool bHasTimeStop = false;
	bool bHasLockOn = false;
};
