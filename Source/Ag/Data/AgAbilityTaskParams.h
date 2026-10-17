// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/AgAttackData.h"
#include "Engine/EngineTypes.h"
#include "AgAbilityTaskParams.generated.h"

class ULevelSequence;
struct FGameplayEffectSpec;

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

	/** 자원 효과(Resource) 스펙에 이 비용을 음수로 넣습니다. 다른 자원은 0입니다. */
	void AddToSpec(FGameplayEffectSpec& Spec) const;
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

	/** 소모 효과(ResourceDrain) 스펙에 한 주기에 줄일 양(초당 소모량 × 스펙의 주기)을 음수로 넣습니다. 다른 자원은 0입니다. */
	void AddToSpec(FGameplayEffectSpec& Spec) const;
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

	/** 자원 효과(Resource) 스펙에 얻는 SP·MP·UP를 넣습니다. */
	void AddToSpec(FGameplayEffectSpec& Spec) const;
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
	FAgTimedTagParams TimedTag;

	UPROPERTY()
	FAgGrantResourcesParams GrantResources;

	UPROPERTY()
	FAgSlowMotionParams SlowMotion;

	UPROPERTY()
	FAgTargetQueryParams TargetQuery;

	/** C++ 사건이 차례대로 치는 단발 타격(처형의 찌르기와 마지막 일격, 궁극기 마무리, 도약 착지 타격) */
	UPROPERTY()
	TArray<FAgAttackParams> Attack;

	UPROPERTY()
	FAgHoldTagParams HoldTag;

	UPROPERTY()
	FAgCollisionResponseParams SetCollisionResponseToChannel;

	UPROPERTY()
	FAgPlayCutsceneParams PlayCutscene;
	bool bHasPlayMontageAndWait = false;
	bool bHasDrainResource = false;
	bool bHasLaunchVertical = false;
	bool bHasLaunchToTarget = false;
	bool bHasTimedTag = false;
	bool bHasGrantResources = false;
	bool bHasSlowMotion = false;
	bool bHasTargetQuery = false;
	bool bHasHoldTag = false;
	bool bHasSetCollisionResponseToChannel = false;
	bool bHasPlayCutscene = false;
	bool bHasTimeStop = false;
	bool bHasLockOn = false;
};
