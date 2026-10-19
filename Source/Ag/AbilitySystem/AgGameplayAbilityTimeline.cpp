// Copyright Woogle. All Rights Reserved.
#include "AbilitySystem/AgGameplayAbility.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Core/AgAssetManager.h"
#include "Data/AgMontageTimeline.h"
#include "Data/AgTaskParams.h"
#include "Ag.h"

void UAgGameplayAbility::FAgTimelineTickFunction::ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	if (IsValid(Target) && TickType != LEVELTICK_ViewportsOnly)
	{
		Target->TickTimeline();
	}
}

FString UAgGameplayAbility::FAgTimelineTickFunction::DiagnosticMessage()
{
	return Target->GetFullName() + TEXT("[TimelineTick]");
}

void UAgGameplayAbility::BeginTimeline(const FAnimMontageInstance& Instance)
{
	const UAnimMontage* PlayedMontage = Instance.Montage;
	const FAgMontageTimeline* Timeline = PlayedMontage ? UAgAssetManager::Get().GetMontageTimelines().Find(*PlayedMontage) : nullptr;
	if (!Timeline)
	{
		return;
	}

	// 데이터를 다시 읽어도 이 재생이 끝날 때까지 같은 항목을 쓰도록 복사합니다.
	for (const FAgTimelineSection& Section : Timeline->Sections)
	{
		for (const FAgTimelineItem& Item : Section.Items)
		{
			float Start = 0.f;
			float End = 0.f;
			if (!AgMontageTimeline::ResolveTimes(*PlayedMontage, Section.Name, Item, Start, End))
			{
				UE_LOG(LogAg, Error, TEXT("%s: section %s of the timeline is missing in the montage"), *GetNameSafe(PlayedMontage),
					*Section.Name.ToString());
				continue;
			}
			FAgTimelineEntry& Entry = TimelineEntries.AddDefaulted_GetRef();
			Entry.Params = Item.Params;
			Entry.bWindow = Item.bWindow;
			Entry.Start = Start;
			Entry.End = End;
		}
	}
	TimelinePosition = Instance.GetPosition();
	TimelineTick.SetTickFunctionEnable(true);
	EvaluateTimeline(TimelinePosition, EAgTimelineMode::Jump);
}

void UAgGameplayAbility::TickTimeline()
{
	if (TimelineEntries.IsEmpty() || !IsActive())
	{
		return;
	}
	UAnimInstance* Anim = CurrentActorInfo ? CurrentActorInfo->GetAnimInstance() : nullptr;
	const FAnimMontageInstance* Instance = Anim ? Anim->GetMontageInstanceForID(MontageInstanceId) : nullptr;
	if (!Instance)
	{
		EndTimeline();
		return;
	}
	EvaluateTimeline(Instance->GetPosition(), EAgTimelineMode::Advance);
}

void UAgGameplayAbility::EvaluateTimeline(float NewPosition, EAgTimelineMode Mode)
{
	enum class EPhase : uint8
	{
		Begin,
		End,
		Point
	};
	struct FPending
	{
		float Time = 0.f;
		EPhase Phase = EPhase::Point;
		int32 Entry = INDEX_NONE;
	};
	TArray<FPending> Pending;

	// 재생 시작과 섹션 이동의 위치는 섹션 시작 시각 그대로라 섹션 0초 지점의 시각과 정확히 같습니다.
	const float Previous = TimelinePosition;
	const bool bForward = Mode == EAgTimelineMode::Advance && NewPosition >= Previous;
	for (int32 Index = 0; Index < TimelineEntries.Num(); ++Index)
	{
		const FAgTimelineEntry& Entry = TimelineEntries[Index];
		if (!Entry.bWindow)
		{
			const bool bReached = bForward ? (Previous < Entry.Start && Entry.Start <= NewPosition)
			                               : (Mode == EAgTimelineMode::Jump && Entry.Start == NewPosition);
			if (bReached)
			{
				Pending.Add(FPending{Entry.Start, EPhase::Point, Index});
			}
			continue;
		}
		if (bForward && !Entry.bActive && Previous < Entry.Start && Entry.Start <= NewPosition)
		{
			Pending.Add(FPending{Entry.Start, EPhase::Begin, Index});
			if (Entry.End <= NewPosition)
			{
				Pending.Add(FPending{Entry.End, EPhase::End, Index});
			}
			continue;
		}
		if (bForward && Entry.bActive && Entry.End <= NewPosition)
		{
			Pending.Add(FPending{Entry.End, EPhase::End, Index});
			continue;
		}
		const bool bInside = Entry.Start <= NewPosition && NewPosition < Entry.End;
		if (Entry.bActive != bInside)
		{
			Pending.Add(FPending{NewPosition, bInside ? EPhase::Begin : EPhase::End, Index});
		}
	}
	TimelinePosition = NewPosition;
	Pending.Sort([](const FPending& A, const FPending& B)
	{
		return A.Time < B.Time;
	});

	const int32 InstanceId = MontageInstanceId;
	for (const FPending& Item : Pending)
	{
		// 항목 실행이 재생을 끝내거나 바꿀 수 있으므로 매번 확인합니다. 항목 실행 안에서 전환해 위치가 바뀌었으면
		// 옮긴 위치의 판정이 이미 끝났으므로 남은 항목은 실행하지 않습니다. 그래서 전환 지점과 같은 시각에는 다른
		// 항목을 두지 않습니다.
		if (!IsActive() || MontageInstanceId != InstanceId || TimelinePosition != NewPosition)
		{
			return;
		}
		FAgTimelineEntry& Entry = TimelineEntries[Item.Entry];
		switch (Item.Phase)
		{
			case EPhase::Begin:
				Entry.bActive = true;
				StartTimelineWindow(Item.Entry);
				break;
			case EPhase::End:
				Entry.bActive = false;
				EndTimelineWindow(Item.Entry);
				break;
			case EPhase::Point:
				RunTimelinePoint(Item.Entry);
				break;
		}
	}
}

void UAgGameplayAbility::EndTimeline()
{
	// Task가 끝나며 다시 재생해도 버린 시간표에 닿지 않도록, 켜진 구간의 Task를 모아 시간표를 버린 뒤 끝냅니다.
	TArray<UAbilityTask*> Active;
	for (const FAgTimelineEntry& Entry : TimelineEntries)
	{
		if (Entry.Task)
		{
			Active.Add(Entry.Task);
		}
	}
	StopTimeline();
	for (UAbilityTask* Task : Active)
	{
		if (!Task->IsFinished())
		{
			Task->EndTask();
		}
	}
}

void UAgGameplayAbility::StopTimeline()
{
	TimelineEntries.Reset();
	TimelinePosition = 0.f;
	TimelineTick.SetTickFunctionEnable(false);
}

void UAgGameplayAbility::StartTimelineWindow(int32 Entry)
{
	FAgTimelineEntry& Item = TimelineEntries[Entry];
	UAbilityTask* Task = Item.Params.Get<FAgTaskParams>().MakeTask(
		FAgTaskContext{*this, EAgTaskPlacement::Window, MontageInstanceId, Item.Start, Item.End});
	if (!Task)
	{
		return;
	}
	// 시작하는 동안 전환하면 시간표가 바뀔 수 있으므로 항목에는 시작하기 전에 둡니다.
	Item.Task = Task;
	Task->ReadyForActivation();
}

void UAgGameplayAbility::EndTimelineWindow(int32 Entry)
{
	UAbilityTask* Task = TimelineEntries[Entry].Task;
	TimelineEntries[Entry].Task = nullptr;
	if (Task && !Task->IsFinished())
	{
		Task->EndTask();
	}
}

void UAgGameplayAbility::RunTimelinePoint(int32 Entry)
{
	// 지점 Task는 재생이 바뀌어도 남고, 행동이 끝날 때 엔진이 끝냅니다.
	const FAgTimelineEntry& Item = TimelineEntries[Entry];
	if (UAbilityTask* Task = Item.Params.Get<FAgTaskParams>().MakeTask(
			FAgTaskContext{*this, EAgTaskPlacement::Point, MontageInstanceId, Item.Start, Item.Start}))
	{
		Task->ReadyForActivation();
	}
}
