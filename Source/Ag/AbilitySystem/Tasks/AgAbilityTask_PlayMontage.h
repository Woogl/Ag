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

	float Start = 0.f;
	float End = 0.f;
	bool bWindow = false;
	bool bActive = false;
};

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

	void RequestTransition(FName Section, const TOptional<float>& BlendTime);

private:
	enum class EAgTimelineMode : uint8
	{
		Advance,
		Jump
	};

	bool OwnsMontage(int32 InstanceId) const
	{
		return !IsFinished() && InstanceId != INDEX_NONE && MontageInstanceId == InstanceId;
	}

	void Play(FName Section, float BlendTime);
	void JumpToSection(FName Section);
	void CancelAbility();

	void HandleMontageBlendingOut(UAnimMontage* EndedMontage, bool bInterrupted, int32 InstanceId);
	void HandleMontageEnded(UAnimMontage* EndedMontage, bool bInterrupted, int32 InstanceId);
	void FinishPlayback(bool bInterrupted);

	void BeginTimeline(const FAnimMontageInstance& Instance);
	void EvaluateTimeline(float NewPosition, EAgTimelineMode Mode);
	void EndTimeline();
	void StopTimeline();
	void StartTimelineWindow(int32 Entry);
	void EndTimelineWindow(int32 Entry);
	void RunTimelinePoint(int32 Entry);
	void EndPointTasks();

	UPROPERTY()
	FAgPlayMontageParams Params;

	UPROPERTY()
	TObjectPtr<UAnimMontage> Montage;

	int32 MontageInstanceId = INDEX_NONE;

	UPROPERTY()
	TArray<FAgTimelineEntry> TimelineEntries;

	UPROPERTY()
	TArray<TObjectPtr<UAbilityTask>> PointTasks;

	float TimelinePosition = 0.f;
};
