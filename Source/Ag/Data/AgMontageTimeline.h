// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/AgAbilityTaskParams.h"
#include "RootMotionModifier.h"
#include "AgMontageTimeline.generated.h"

class FJsonObject;
class UAnimMontage;

/** 투사체를 쏘는 위치 */
UENUM()
enum class EAgProjectileOrigin : uint8
{
	CharacterFeetForward,
	MeshSocket
};

/** 공격 판정 구간. 값은 한 타의 공격 데이터입니다. */
USTRUCT()
struct FAgTimelineAttack : public FAgAttackParams
{
	GENERATED_BODY()

	UPROPERTY()
	float Start = 0.f;

	UPROPERTY()
	float End = 0.f;
};

/** 태그를 유지하는 구간 */
USTRUCT()
struct FAgTimelineHoldTag : public FAgHoldTagParams
{
	GENERATED_BODY()

	UPROPERTY()
	float Start = 0.f;

	UPROPERTY()
	float End = 0.f;
};

/** 캡슐의 충돌 응답을 바꾸는 구간 */
USTRUCT()
struct FAgTimelineCollisionResponse : public FAgCollisionResponseParams
{
	GENERATED_BODY()

	UPROPERTY()
	float Start = 0.f;

	UPROPERTY()
	float End = 0.f;
};

/** 돌진 구간 */
USTRUCT()
struct FAgTimelineDash : public FAgDashParams
{
	GENERATED_BODY()

	UPROPERTY()
	float Start = 0.f;

	UPROPERTY()
	float End = 0.f;
};

/** 엔진 Motion Warping 구간. 최대 워핑 거리 0은 위치 보정 없이 회전만 합니다. */
USTRUCT()
struct FAgTimelineMotionWarping
{
	GENERATED_BODY()

	UPROPERTY()
	float Start = 0.f;

	UPROPERTY()
	float End = 0.f;

	UPROPERTY()
	FAgMotionWarpingParams Params;

	UPROPERTY()
	EMotionWarpRotationMethod RotationMethod = EMotionWarpRotationMethod::ConstantRate;
};

/** 재생을 소유한 어빌리티에 보내는 사건 */
USTRUCT()
struct FAgTimelineGameplayEvent
{
	GENERATED_BODY()

	UPROPERTY()
	float Time = 0.f;

	UPROPERTY()
	FGameplayTag Event;
};

/** 투사체 한 발. Attack은 그 투사체의 공격 데이터입니다. */
USTRUCT()
struct FAgTimelineSpawnProjectile
{
	GENERATED_BODY()

	UPROPERTY()
	float Time = 0.f;

	UPROPERTY()
	FName ProjectileId;

	UPROPERTY()
	EAgProjectileOrigin Origin = EAgProjectileOrigin::CharacterFeetForward;

	UPROPERTY()
	FName SocketName;

	UPROPERTY()
	FAgAttackParams Attack;
};

/** 몽타주 하나의 시간표. 시각은 몽타주 타임라인의 초(재생 위치)입니다. */
USTRUCT()
struct FAgMontageTimeline
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FAgTimelineAttack> Attack;

	UPROPERTY()
	TArray<FAgTimelineHoldTag> HoldTag;

	UPROPERTY()
	TArray<FAgTimelineCollisionResponse> SetCollisionResponseToChannel;

	UPROPERTY()
	TArray<FAgTimelineDash> Dash;

	UPROPERTY()
	TArray<FAgTimelineMotionWarping> MotionWarping;

	UPROPERTY()
	TArray<FAgTimelineGameplayEvent> GameplayEvent;

	UPROPERTY()
	TArray<FAgTimelineSpawnProjectile> SpawnProjectile;
};

/** MontageTimelines.json의 값. 키는 몽타주 에셋 경로입니다. */
UCLASS()
class UAgMontageTimelineData : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, VisibleAnywhere, Category = "Timeline")
	TMap<FName, FAgMontageTimeline> Timelines;

	/** 몽타주의 시간표. 없으면 null입니다. 데이터를 다시 읽으면 바뀌므로 포인터는 그 함수 안에서만 씁니다. */
	const FAgMontageTimeline* Find(const UAnimMontage& Montage) const;
};

namespace AgMontageTimeline
{
	/** 고정 스키마만 읽습니다. 실패하면 Out을 바꾸지 않습니다. */
	bool Parse(const FJsonObject& Json, TMap<FName, FAgMontageTimeline>& Out, TArray<FString>& Errors);

#if WITH_EDITOR
	/** 몽타주 에셋과 대조합니다: 길이, 반복 섹션, 겹치는 제어, 몽타주에 남긴 노티파이와의 시각 */
	void ValidateAssets(const TMap<FName, FAgMontageTimeline>& Timelines, TArray<FString>& Errors);
#endif
}
