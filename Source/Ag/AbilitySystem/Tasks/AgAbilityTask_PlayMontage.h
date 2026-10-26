// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgTaskParams.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "AgAbilityTask_PlayMontage.generated.h"

class UAnimMontage;
class UAgGameplayAbility;
struct FAnimMontageInstance;

// 정면에서 시계 방향으로 45°씩이며, 값의 순서가 곧 각도입니다.
UENUM()
enum class EAgMoveDirection : uint8
{
	Forward,
	ForwardRight,
	Right,
	BackwardRight,
	Backward,
	BackwardLeft,
	Left,
	ForwardLeft
};

USTRUCT()
struct FAgPlayMontageParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	FName Montage;

	UPROPERTY()
	FName Section;

	UPROPERTY()
	FGameplayTagContainer RequiredTags;

	UPROPERTY()
	FGameplayTagContainer BlockedTags;

	UPROPERTY()
	TOptional<EAgMoveDirection> MoveDirection;

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

USTRUCT()
struct FAgTimelineEntry
{
	GENERATED_BODY()

	UPROPERTY()
	TInstancedStruct<FAgTaskParams> Params;

	UPROPERTY()
	TObjectPtr<UAbilityTask> Task;

	/** Params의 Start·End(섹션 기준)와 달리 몽타주 기준 시각입니다. */
	float Start = 0.f;
	float End = 0.f;
	bool bActive = false;
};

// 엔진 PlayMontageAndWait에는 블렌드 인 시간을 정하는 길이 없고 섹션 이동과 다시 재생을 한 Task로 이어 갈 수 없어 상속하지 않습니다.
UCLASS()
class UAgAbilityTask_PlayMontage : public UAbilityTask
{
	GENERATED_BODY()
	friend struct FAgPlayMontageParams;

public:
	UAgAbilityTask_PlayMontage();

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

	void JumpToSection(FName Section);
	void ReplaySection(FName Section, float BlendTime);

	/** 재생이 바뀌면 구간 Task가 끝나므로, 구간 Task가 켜져 있는 동안은 그 구간의 재생입니다. */
	const FAnimMontageInstance* GetMontageInstance() const;

private:
	enum class EAgTimelineMode : uint8
	{
		Advance,
		Jump
	};

	bool IsCurrentMontageInstance(int32 InstanceId) const
	{
		return !IsFinished() && InstanceId != INDEX_NONE && MontageInstanceId == InstanceId;
	}

	void Play(FName Section, float BlendTime);
	void CancelAbility();

	void HandleMontageBlendingOut(UAnimMontage* EndedMontage, bool bInterrupted, int32 InstanceId);
	void HandleMontageEnded(UAnimMontage* EndedMontage, bool bInterrupted, int32 InstanceId);
	void FinishPlayback(bool bInterrupted);

	void BeginTimeline(const FAnimMontageInstance& Instance);
	void EvaluateTimeline(float NewPosition, EAgTimelineMode Mode);
	void EndTimeline();
	void ResetTimeline();
	void StartTimelineWindow(int32 Entry);
	void EndTimelineWindow(int32 Entry);

	UPROPERTY()
	FAgPlayMontageParams Params;

	UPROPERTY()
	TObjectPtr<UAnimMontage> Montage;

	/**
	 * 지금 재생의 ID로, 콜백이 이 재생의 것인지 가리고 끝날 때 멈출 재생을 찾습니다. INDEX_NONE이면 전환 요청을
	 * 받지 않으므로, 끝나는 구간이 요청하는 전환을 거르도록 구간을 끝내기 전에 비웁니다.
	 */
	int32 MontageInstanceId = INDEX_NONE;

	UPROPERTY()
	TArray<FAgTimelineEntry> TimelineEntries;

	float TimelinePosition = 0.f;
};
