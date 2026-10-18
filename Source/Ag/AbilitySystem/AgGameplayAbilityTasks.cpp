// Copyright Woogle. All Rights Reserved.
#include "AbilitySystem/AgGameplayAbility.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Animation/AgRootMotionModifier_MotionWarping.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/AgCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/AgAssetManager.h"
#include "Data/AgMontageTimeline.h"
#include "Data/AgTaskParams.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MotionWarpingComponent.h"
#include "Ag.h"

namespace
{
	/** 재생 시작과 섹션 이동에서 그 위치의 지점으로 보는, 위치보다 앞쪽의 차이. 위치보다 뒤의 지점은 다음 진행이 실행합니다. */
	constexpr float PointTolerance = 1.e-4f;
}

void UAgGameplayAbility::FAgTimelineTickFunction::ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread,
                                                              const FGraphEventRef& MyCompletionGraphEvent)
{
	if (IsValid(Target) && TickType != LEVELTICK_ViewportsOnly)
	{
		Target->TickTimeline(DeltaTime);
	}
}

FString UAgGameplayAbility::FAgTimelineTickFunction::DiagnosticMessage()
{
	return Target->GetFullName() + TEXT("[TimelineTick]");
}

void UAgGameplayAbility::BeginTimeline(const FAnimMontageInstance& Instance)
{
	AAgCharacterBase* Character = GetAgCharacter();
	const UAnimMontage* PlayedMontage = Instance.Montage;
	const FAgMontageTimeline* Timeline = PlayedMontage ? UAgAssetManager::Get().GetMontageTimelines().Find(*PlayedMontage) : nullptr;
	if (!Character || !Timeline)
	{
		return;
	}

	// 데이터를 다시 읽어도 이 재생이 끝날 때까지 같은 항목을 쓰도록 복사하고, 섹션 기준 시각을 몽타주 시각으로 바꿉니다.
	EndTimeline();
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

void UAgGameplayAbility::TickTimeline(float DeltaTime)
{
	if (TimelineInstanceId == INDEX_NONE || !IsActive() || bEnding)
	{
		return;
	}
	UAnimInstance* Anim = GetActorInfo().GetAnimInstance();
	const int32 InstanceId = TimelineInstanceId;
	const float Previous = TimelinePosition;
	const FAnimMontageInstance* Instance = Anim ? Anim->GetMontageInstanceForID(InstanceId) : nullptr;
	if (!Instance)
	{
		EndTimeline();
		return;
	}
	const float Position = Instance->GetPosition();
	EvaluateTimeline(Position, ETimelineMode::Advance);

	// 판정이 재생을 바꾸거나 섹션을 옮기지 않았으면 몽타주 루트 모션이 없는 추적 구간을 돌립니다.
	const bool bSamePlay = TimelineInstanceId == InstanceId && TimelinePosition == Position && IsActive() && !bEnding;
	Instance = bSamePlay ? Anim->GetMontageInstanceForID(InstanceId) : nullptr;
	if (Instance)
	{
		TurnTracks(*Instance, Previous, DeltaTime);
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
			// 추적 구간은 워핑 컴포넌트가 루트 모션을 처리할 때 등록하거나(HandleWarpingPreUpdate), 판정 뒤에 직접 돌립니다(TurnTracks).
			continue;
		}
		if (!Entry.bWindow)
		{
			const bool bReached =
			    bForward ? (Previous < Entry.Start && Entry.Start <= NewPosition)
			             : (Mode != ETimelineMode::Advance && Entry.Start <= NewPosition && NewPosition - Entry.Start <= PointTolerance);
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
		// 항목 실행이 재생을 끝내거나 바꿀 수 있으므로 매번 확인합니다. 항목 실행 안에서 전환해 위치가 바뀌었으면 옮긴 위치의
		// 판정이 이미 끝났으므로 남은 항목은 실행하지 않습니다.
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
	UAbilityTask* Task =
	    TimelineEntries[Entry].Params.Get<FAgTaskParams>().MakeTask(FAgTaskContext{*this, EAgTaskPlacement::Window, InstanceId});
	if (!Task)
	{
		return;
	}
	FAgAbilityWindowExecution& Execution = Windows.AddDefaulted_GetRef();
	Execution.Task = Task;
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
	// 지점 Task는 재생이 바뀌어도 남고, 행동이 끝날 때 엔진이 끝냅니다.
	if (UAbilityTask* Task =
	        TimelineEntries[Entry].Params.Get<FAgTaskParams>().MakeTask(FAgTaskContext{*this, EAgTaskPlacement::Point, TimelineInstanceId}))
	{
		Task->ReadyForActivation();
	}
}

void UAgGameplayAbility::EndWindows()
{
	TArray<FAgAbilityWindowExecution> Ending = MoveTemp(Windows);
	Windows.Reset();
	for (FAgAbilityWindowExecution& Window : Ending)
	{
		if (Window.Task && !Window.Task->IsFinished())
		{
			Window.Task->EndTask();
		}
	}
}

void UAgGameplayAbility::ClearWarps()
{
	TArray<TObjectPtr<UAgRootMotionModifier_MotionWarping>> Previous = MoveTemp(Warps);
	Warps.Reset();
	for (UAgRootMotionModifier_MotionWarping* Warp : Previous)
	{
		if (Warp)
		{
			Warp->SetState(ERootMotionModifierState::MarkedForRemoval);
		}
	}
}

void UAgGameplayAbility::TurnTracks(const FAnimMontageInstance& Instance, float PreviousPosition, float DeltaTime)
{
	// 몽타주가 루트 모션을 주는 동안은 엔진 Motion Warping이 돌립니다(HandleWarpingPreUpdate).
	AAgCharacterBase* Character = GetAgCharacter();
	if (!Character || !Montage || (Montage->HasRootMotion() && !Instance.IsRootMotionDisabled()))
	{
		return;
	}
	const float Position = Instance.GetPosition();
	for (FAgTimelineEntry& Entry : TimelineEntries)
	{
		const FAgTrackParams* Track = Entry.Params.GetPtr<FAgTrackParams>();
		if (!Track)
		{
			continue;
		}

		// 엔진 워핑처럼 이번 진행이 구간 안에서 시작했을 때 돕니다.
		if (PreviousPosition < Entry.Start || PreviousPosition >= Entry.End)
		{
			Entry.bTrackStarted = Entry.bTrackStopped = false;
			continue;
		}
		if (!Entry.bTrackStarted)
		{
			Entry.bTrackStarted = true;
			AActor* Target = nullptr;
			float Gap = 0.f;
			bool bFacingOnly = false;
			Entry.bTrackStopped = !PrepareTrackTarget(MontageInstanceId, *Track, Target, Gap, bFacingOnly);
			Entry.TrackTarget = Target;
			Entry.bTrackFacingOnly = bFacingOnly;
		}

		// 따라 돌다가 대상이 TargetingRange 밖으로 나가면 그 구간에서는 멈춥니다.
		const AActor* Target = Entry.TrackTarget.Get();
		Entry.bTrackStopped |= !Target || (!Entry.bTrackFacingOnly && Track->bHasTargetingRange &&
		                                   Character->GetHorizontalDistanceTo(Target) > Track->TargetingRange);
		const FVector ToTarget =
		    Target ? (Target->GetActorLocation() - Character->GetActorLocation()).GetSafeNormal2D() : FVector::ZeroVector;
		if (Entry.bTrackStopped || ToTarget.IsNearlyZero())
		{
			continue;
		}

		// ConstantRate는 주인의 시간으로 초당 캐릭터 회전 속도만큼, Slerp는 구간 끝에 다 돌도록 남은 각도를 나눠 돕니다.
		const float CurrentYaw = static_cast<float>(Character->GetActorRotation().Yaw);
		const float GoalYaw = static_cast<float>(ToTarget.Rotation().Yaw);
		float Yaw = CurrentYaw;
		if (Track->RotationMethod == EMotionWarpRotationMethod::Slerp)
		{
			const float Remaining = Entry.End - PreviousPosition;
			const float Alpha = Remaining > UE_SMALL_NUMBER
			                        ? FMath::Clamp((FMath::Min(Position, Entry.End) - PreviousPosition) / Remaining, 0.f, 1.f)
			                        : 1.f;
			Yaw = CurrentYaw + FMath::FindDeltaAngleDegrees(CurrentYaw, GoalYaw) * Alpha;
		}
		else
		{
			const float MaxTurn =
			    static_cast<float>(Character->GetCharacterMovement()->RotationRate.Yaw) * DeltaTime * Character->CustomTimeDilation;
			Yaw = FMath::FixedTurn(CurrentYaw, GoalYaw, MaxTurn);
		}
		FRotator Rotation = Character->GetActorRotation();
		Rotation.Yaw = Yaw;
		Character->SetActorRotation(Rotation);
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
	const UAnimMontage* PlayedMontage = Instance->Montage.Get();
	const float Previous = Instance->GetPreviousPosition();
	for (const FAgTimelineEntry& Entry : TimelineEntries)
	{
		const FAgTrackParams* Track = Entry.Params.GetPtr<FAgTrackParams>();
		if (Track && Entry.Start <= Previous && Previous < Entry.End && !Warping->ContainsModifier(PlayedMontage, Entry.Start, Entry.End))
		{
			UAgRootMotionModifier_MotionWarping* Modifier = NewObject<UAgRootMotionModifier_MotionWarping>(Warping);
			Modifier->Animation = PlayedMontage;
			Modifier->StartTime = Entry.Start;
			Modifier->EndTime = Entry.End;
			Modifier->SetTrackSettings(*Track);
			Warping->AddModifier(Modifier);
		}
	}
}

bool UAgGameplayAbility::PrepareTrackTarget(int32 InstanceId, const FAgTrackParams& Params, AActor*& Target, float& Gap,
                                            bool& bFacingOnly) const
{
	const AAgCharacterBase* Character = GetAgCharacter();
	if (!OwnsMontage(InstanceId) || !Character)
	{
		return false;
	}
	Target = ActionTarget.Get();
	Gap = StopDistance;
	bFacingOnly = false;
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
