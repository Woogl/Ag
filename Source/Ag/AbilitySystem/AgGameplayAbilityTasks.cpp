// Copyright Woogle. All Rights Reserved.
#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystem/Tasks/AgAbilityTask_Dash.h"
#include "Animation/AgRootMotionModifier_MotionWarping.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/AgCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/AgAssetManager.h"
#include "Data/AgMontageTimeline.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MotionWarpingComponent.h"
#include "Ag.h"

namespace
{
	/** 재생 시작과 섹션 이동에서 그 위치의 지점으로 보는 차이 */
	constexpr float PointTolerance = 1.e-4f;
}

void UAgGameplayAbility::FAgTimelineTickFunction::ExecuteTick(
    float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
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
	AAgCharacterBase* Character = GetAgCharacter();
	const UAnimMontage* Montage = Instance.Montage;
	const FAgMontageTimeline* Timeline = Montage ? UAgAssetManager::Get().GetMontageTimelines().Find(*Montage) : nullptr;
	if (!Character || !Timeline)
	{
		return;
	}

	// 데이터를 다시 읽어도 이 재생이 끝날 때까지 같은 항목을 쓰도록 복사하고, 섹션 기준 시각을 몽타주 시각으로 바꿉니다.
	StopTimeline();
	for (const FAgTimelineSection& Section : Timeline->Sections)
	{
		for (const FAgTimelineItem& Item : Section.Items)
		{
			float Start = 0.f;
			float End = 0.f;
			if (!AgMontageTimeline::ResolveTimes(*Montage, Section.Name, Item, Start, End))
			{
				UE_LOG(LogAg, Error, TEXT("%s: section %s of the timeline is missing in the montage"), *GetNameSafe(Montage),
				    *Section.Name.ToString());
				continue;
			}
			FAgTimelineEntry& Entry = TimelineEntries.AddDefaulted_GetRef();
			Entry.Params = Item.Params;
			Entry.Task = Item.Task;
			Entry.bWindow = Item.bWindow;
			Entry.Start = Start;
			Entry.End = End;
		}
	}
	TimelineInstanceId = Instance.GetInstanceID();
	TimelinePosition = Instance.GetPosition();

	// 몽타주가 진행된 뒤(루트 모션 몽타주는 이동 틱 안에서) 판정해, 같은 프레임의 공격 판정(TG_PostPhysics)보다 먼저 구간을 엽니다.
	UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	USkeletalMeshComponent* Mesh = Character->GetMesh();
	TimelineTick.AddPrerequisite(Movement, Movement->PrimaryComponentTick);
	TimelineTick.AddPrerequisite(Mesh, Mesh->PrimaryComponentTick);
	if (!TimelineTick.IsTickFunctionRegistered())
	{
		TimelineTick.RegisterTickFunction(Character->GetLevel());
	}
	TimelineTick.SetTickFunctionEnable(true);
	if (UMotionWarpingComponent* Warping = Character->GetMotionWarping())
	{
		Warping->OnPreUpdate.AddUniqueDynamic(this, &ThisClass::HandleWarpingPreUpdate);
	}
	EvaluateTimeline(TimelinePosition, ETimelineMode::Start);
}

void UAgGameplayAbility::ResyncTimeline()
{
	UAnimInstance* Anim = TimelineInstanceId != INDEX_NONE ? GetActorInfo().GetAnimInstance() : nullptr;
	if (const FAnimMontageInstance* Instance = Anim ? Anim->GetMontageInstanceForID(TimelineInstanceId) : nullptr)
	{
		EvaluateTimeline(Instance->GetPosition(), ETimelineMode::Jump);
	}
}

void UAgGameplayAbility::TickTimeline()
{
	if (TimelineInstanceId == INDEX_NONE || !IsActive())
	{
		return;
	}
	UAnimInstance* Anim = GetActorInfo().GetAnimInstance();
	if (const FAnimMontageInstance* Instance = Anim ? Anim->GetMontageInstanceForID(TimelineInstanceId) : nullptr)
	{
		EvaluateTimeline(Instance->GetPosition(), ETimelineMode::Advance);
	}
	else
	{
		EndTimeline();
	}
}

void UAgGameplayAbility::EvaluateTimeline(float NewPosition, ETimelineMode Mode)
{
	// 같은 시각이면 구간 끝, 지점, 구간 시작 순서이고, 그래도 같으면 항목 순서입니다.
	enum class EPhase : uint8
	{
		End,
		Point,
		Begin
	};
	struct FPending
	{
		float Time = 0.f;
		EPhase Phase = EPhase::Point;
		int32 Entry = INDEX_NONE;
	};
	TArray<FPending> Pending;
	const TFunction<void(float, EPhase, int32)> Add = [&Pending](float Time, EPhase Phase, int32 Entry)
	{
		FPending& Item = Pending.AddDefaulted_GetRef();
		Item.Time = Time;
		Item.Phase = Phase;
		Item.Entry = Entry;
	};

	// 앞으로 진행하면 지나간 구간 경계와 지점을 모두 실행합니다. 재생 시작, 섹션 이동, 반복으로 위치가 뒤로 간 경우는
	// 위치로만 구간을 맞추고, 시작과 섹션 이동에서는 그 위치의 지점만 실행합니다.
	const float Previous = TimelinePosition;
	const bool bForward = Mode == ETimelineMode::Advance && NewPosition >= Previous;
	for (int32 Index = 0; Index < TimelineEntries.Num(); ++Index)
	{
		const FAgTimelineEntry& Entry = TimelineEntries[Index];
		if (Entry.Params.GetPtr<FAgTrackParams>())
		{
			// 추적 구간은 워핑 컴포넌트가 루트 모션을 처리할 때 등록합니다(HandleWarpingPreUpdate).
			continue;
		}
		if (!Entry.bWindow)
		{
			const bool bReached = bForward
			                          ? (Previous < Entry.Start && Entry.Start <= NewPosition)
			                          : (Mode != ETimelineMode::Advance && FMath::IsNearlyEqual(Entry.Start, NewPosition, PointTolerance));
			if (bReached)
			{
				Add(Entry.Start, EPhase::Point, Index);
			}
			continue;
		}
		if (bForward && !Entry.bActive && Previous < Entry.Start && Entry.Start <= NewPosition)
		{
			// 한 번의 판정에서 구간 전체를 지나면 열고 바로 닫습니다.
			Add(Entry.Start, EPhase::Begin, Index);
			if (Entry.End <= NewPosition)
			{
				Add(Entry.End, EPhase::End, Index);
			}
			continue;
		}
		if (bForward && Entry.bActive && Entry.End <= NewPosition)
		{
			Add(Entry.End, EPhase::End, Index);
			continue;
		}
		const bool bInside = Entry.Start <= NewPosition && NewPosition < Entry.End;
		if (Entry.bActive != bInside)
		{
			Add(NewPosition, bInside ? EPhase::Begin : EPhase::End, Index);
		}
	}
	TimelinePosition = NewPosition;
	Pending.StableSort(
	    [](const FPending& A, const FPending& B)
	    {
		    if (A.Time != B.Time)
		    {
			    return A.Time < B.Time;
		    }
		    if (A.Phase != B.Phase)
		    {
			    return A.Phase < B.Phase;
		    }
		    return A.Entry < B.Entry;
	    });

	const int32 InstanceId = TimelineInstanceId;
	for (const FPending& Item : Pending)
	{
		// 항목 실행이 재생을 끝내거나 바꿀 수 있으므로 매번 확인합니다. 항목 실행 안에서 섹션을 옮겨 위치가 바뀌었으면 옮긴
		// 위치의 판정이 이미 끝났으므로 남은 항목은 실행하지 않습니다.
		if (!IsActive() || bEnding || TimelineInstanceId != InstanceId || TimelinePosition != NewPosition)
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
			EndTimelineWindow(InstanceId, Item.Entry);
			break;
		case EPhase::Point:
			RunTimelinePoint(Item.Entry);
			break;
		}
	}
}

void UAgGameplayAbility::EndTimeline()
{
	const int32 InstanceId = TimelineInstanceId;
	TArray<int32> Active;
	for (int32 Index = 0; Index < TimelineEntries.Num(); ++Index)
	{
		if (TimelineEntries[Index].bActive)
		{
			Active.Add(Index);
		}
	}
	StopTimeline();
	for (int32 Index : Active)
	{
		EndTimelineWindow(InstanceId, Index);
	}
}

void UAgGameplayAbility::StopTimeline()
{
	TimelineEntries.Reset();
	TimelineInstanceId = INDEX_NONE;
	TimelinePosition = 0.f;
	TimelineTick.SetTickFunctionEnable(false);
	const AAgCharacterBase* Character = GetAgCharacter();
	if (UMotionWarpingComponent* Warping = Character ? Character->GetMotionWarping() : nullptr)
	{
		Warping->OnPreUpdate.RemoveDynamic(this, &ThisClass::HandleWarpingPreUpdate);
	}
}

void UAgGameplayAbility::StartTimelineWindow(int32 Entry)
{
	const int32 InstanceId = TimelineInstanceId;
	if (Windows.ContainsByPredicate(
	        [=](const FAgAbilityWindowExecution& E)
	        {
		        return E.MontageInstanceId == InstanceId && E.Entry == Entry;
	        }))
	{
		return;
	}
	const FAgTimelineEntry& Item = TimelineEntries[Entry];
	const FName TaskName = Item.Task;
	if (GetAbilityId() == TEXT("DashThrust") && bDashFinished &&
	    (TaskName == TEXT("Dash") || TaskName == TEXT("Attack") || TaskName == TEXT("SetCollisionResponseToChannel")))
	{
		return;
	}
	UAbilityTask* Task = Item.Params.Get<FAgTaskParams>().MakeTask(FAgTaskContext{*this, InstanceId});
	if (!Task)
	{
		return;
	}
	if (UAgAbilityTask_Dash* Dash = Cast<UAgAbilityTask_Dash>(Task))
	{
		DashTask = Dash;
		if (GetAbilityId() == TEXT("DashThrust"))
		{
			Dash->OnFinished.AddDynamic(this, &ThisClass::HandleDashFinished);
		}
	}
	FAgAbilityWindowExecution& Execution = Windows.AddDefaulted_GetRef();
	Execution.Task = Task;
	Execution.TaskName = TaskName;
	Execution.MontageInstanceId = InstanceId;
	Execution.Entry = Entry;
	Task->ReadyForActivation();
}

void UAgGameplayAbility::EndTimelineWindow(int32 InstanceId, int32 Entry)
{
	const int32 Index = Windows.IndexOfByPredicate(
	    [=](const FAgAbilityWindowExecution& E)
	    {
		    return E.MontageInstanceId == InstanceId && E.Entry == Entry;
	    });
	if (Index == INDEX_NONE)
	{
		return;
	}
	UAbilityTask* Task = Windows[Index].Task;
	Windows.RemoveAt(Index);
	if (Task && !Task->IsFinished())
	{
		Task->EndTask();
	}
}

void UAgGameplayAbility::RunTimelinePoint(int32 Entry)
{
	const FAgTimelineEntry& Item = TimelineEntries[Entry];
	if (const FAgGameplayEventParams* Event = Item.Params.GetPtr<FAgGameplayEventParams>())
	{
		HandleMontageEvent(TimelineInstanceId, Event->Event);
		return;
	}
	if (UAbilityTask* Task = Item.Params.Get<FAgTaskParams>().MakeTask(FAgTaskContext{*this, TimelineInstanceId}))
	{
		Task->ReadyForActivation();
	}
}

void UAgGameplayAbility::HandleWarpingPreUpdate(UMotionWarpingComponent* Warping)
{
	// 엔진이 워핑 노티파이 구간에 Modifier를 더하는 때와 조건(앞 위치가 구간 안이고 같은 구간의 Modifier가 없음)을 따릅니다.
	// 켜고 지우는 것과 루트 모션 보정은 엔진이 재생 위치로 합니다.
	const AAgCharacterBase* Character = GetAgCharacter();
	const FAnimMontageInstance* Instance = Character ? Character->GetRootMotionAnimMontageInstance() : nullptr;
	if (!Warping || !Instance || Instance->GetInstanceID() != TimelineInstanceId || !IsActive() || bEnding)
	{
		return;
	}
	const UAnimMontage* Montage = Instance->Montage.Get();
	const float Previous = Instance->GetPreviousPosition();
	for (const FAgTimelineEntry& Entry : TimelineEntries)
	{
		const FAgTrackParams* Track = Entry.Params.GetPtr<FAgTrackParams>();
		if (Track && Entry.Start <= Previous && Previous < Entry.End && !Warping->ContainsModifier(Montage, Entry.Start, Entry.End))
		{
			UAgRootMotionModifier_MotionWarping* Modifier = NewObject<UAgRootMotionModifier_MotionWarping>(Warping);
			Modifier->Animation = Montage;
			Modifier->StartTime = Entry.Start;
			Modifier->EndTime = Entry.End;
			Modifier->SetTrackSettings(*Track);
			Warping->AddModifier(Modifier);
		}
	}
}

void UAgGameplayAbility::HandleDashFinished()
{
	if (!IsActive() || bEnding)
	{
		return;
	}
	bDashFinished = true;
	DashTask = nullptr;
	// 기획의 돌진 종료 사건만 자기 행동의 남은 판정·이동·충돌 구간을 닫습니다.
	TArray<TObjectPtr<UAbilityTask>> Ending;
	for (FAgAbilityWindowExecution& Window : Windows)
	{
		if (Window.TaskName == TEXT("Dash") || Window.TaskName == TEXT("Attack") ||
		    Window.TaskName == TEXT("SetCollisionResponseToChannel"))
		{
			Ending.Add(Window.Task);
			Window.Task = nullptr;
		}
	}
	for (UAbilityTask* Task : Ending)
	{
		if (Task && !Task->IsFinished())
		{
			Task->EndTask();
		}
	}
	if (bHoldingDash && Motion.Montage)
	{
		bHoldingDash = false;
		GetActorInfo().GetAnimInstance()->Montage_Resume(Motion.Montage);
	}
}

bool UAgGameplayAbility::PrepareWarpTarget(
    int32 InstanceId, float WindowStart, const FAgTrackParams& Params, AActor*& Target, float& Gap, bool& bFacingOnly)
{
	if (!OwnsMontage(InstanceId) || bEnding)
	{
		return false;
	}
	bFacingOnly = false;
	AAgCharacterBase* Character = GetAgCharacter();
	if (GetAbilityId() == TEXT("Execution"))
	{
		Target = ExecutionTarget.Get();
		Gap = WarpStopDistance;
		return true;
	}
	if (Motion.TargetPhaseStart != WindowStart)
	{
		Motion.TargetPhaseStart = WindowStart;
		Motion.Target = Character->FindAttackTarget();
		UpdateMotionConstraints();
	}
	Target = Motion.Target.Get();
	Gap = WarpStopDistance;
	if (!Target || (Params.bHasTargetingRange && Character->GetHorizontalDistanceTo(Target) > Params.TargetingRange))
	{
		Target = Character->GetFallbackFacingTarget();
		bFacingOnly = true;
	}
	return true;
}

void UAgGameplayAbility::RegisterWarp(UAgRootMotionModifier_MotionWarping* Modifier)
{
	Warps.AddUnique(Modifier);
}
