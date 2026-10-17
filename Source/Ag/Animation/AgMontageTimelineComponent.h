// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AgMontageTimelineComponent.generated.h"

class UAnimInstance;
class UAnimMontage;
class UMotionWarpingComponent;
struct FAgMontageTimeline;
struct FAnimMontageInstance;

/** 어빌리티에 알리는 시간표 항목의 기능. 같은 시각이면 이 순서로 처리합니다. */
enum class EAgTimelineFeature : uint8
{
	Attack,
	HoldTag,
	SetCollisionResponseToChannel,
	Dash,
	GameplayEvent,
	SpawnProjectile
};

/** 같은 시각이면 구간 끝, 지점, 구간 시작 순서로 처리합니다. */
enum class EAgTimelinePhase : uint8
{
	End,
	Point,
	Begin
};

/** 어빌리티에 알리는 시간표 항목 하나. Timeline은 그 재생을 시작할 때 복사한 시간표이며 알림 안에서만 씁니다. */
struct FAgTimelineEvent
{
	int32 InstanceId = INDEX_NONE;
	EAgTimelineFeature Feature = EAgTimelineFeature::Attack;
	EAgTimelinePhase Phase = EAgTimelinePhase::Point;
	int32 Index = INDEX_NONE;
	const FAgMontageTimeline* Timeline = nullptr;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FAgTimelineEventSignature, const FAgTimelineEvent&);

/**
 * 캐릭터에서 재생 중인 몽타주의 시간표를 재생 위치로 판정합니다(구현 계획서 '몽타주 시간표').
 * 워핑 구간은 엔진이 워핑 노티파이를 찾을 때와 같은 때와 조건으로 엔진 Motion Warping에 등록하고, 나머지 항목은 알림으로
 * 보냅니다. 재생을 소유한 어빌리티가 자기 재생의 알림만 처리합니다.
 */
UCLASS()
class UAgMontageTimelineComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAgMontageTimelineComponent();

	/** 어빌리티가 모션을 시작한 직후 부릅니다. 시작 위치에 걸친 구간과 그 위치의 지점을 바로 실행합니다. 판정은 이때부터 합니다. */
	void BeginPlayback(int32 InstanceId);

	/** 어빌리티가 섹션을 옮긴 직후 부릅니다. 건너뛴 지점은 실행하지 않고, 옮긴 위치의 지점은 실행합니다. */
	void Resync(int32 InstanceId);

	/** 몽타주의 시간표. 없으면 null입니다. */
	static const FAgMontageTimeline* FindTimeline(const UAnimMontage& Montage);

	FAgTimelineEventSignature OnTimelineEvent;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	enum class EMode : uint8
	{
		Start,
		Advance,
		Jump
	};

	struct FWindow
	{
		EAgTimelineFeature Feature = EAgTimelineFeature::Attack;
		int32 Index = INDEX_NONE;
		float Start = 0.f;
		float End = 0.f;
		bool bActive = false;
	};

	struct FPoint
	{
		EAgTimelineFeature Feature = EAgTimelineFeature::GameplayEvent;
		int32 Index = INDEX_NONE;
		float Time = 0.f;
	};

	/** 재생 하나의 판정 상태. 시간표는 재생을 시작할 때 복사해 다시 읽어도 그 재생이 끝날 때까지 씁니다. */
	struct FPlayback
	{
		int32 InstanceId = INDEX_NONE;
		TSharedPtr<const FAgMontageTimeline> Timeline;
		TArray<FWindow> Windows;
		TArray<FPoint> Points;
		float Position = 0.f;
	};

	UAnimInstance* GetAnimInstance() const;
	FPlayback* FindPlayback(int32 InstanceId);
	bool StartTracking(const FAnimMontageInstance& Instance);
	void Evaluate(int32 InstanceId, float NewPosition, EMode Mode);
	void EndPlayback(int32 InstanceId);

	UFUNCTION()
	void HandleWarpingPreUpdate(UMotionWarpingComponent* Warping);

	TArray<FPlayback> Playbacks;
};
