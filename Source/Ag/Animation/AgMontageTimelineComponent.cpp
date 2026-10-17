// Copyright Woogle. All Rights Reserved.

#include "Animation/AgMontageTimelineComponent.h"
#include "Animation/AgRootMotionModifier_MotionWarping.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/AgCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/AgAssetManager.h"
#include "Data/AgMontageTimeline.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MotionWarpingComponent.h"

namespace
{
	/** 재생 시작과 섹션 이동에서 그 위치의 지점으로 보는 차이 */
	constexpr float PointTolerance = 1.e-4f;
}

UAgMontageTimelineComponent::UAgMontageTimelineComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UAgMontageTimelineComponent::BeginPlay()
{
	Super::BeginPlay();

	// 몽타주가 진행된 뒤(루트 모션 몽타주는 이동 틱 안에서) 판정해, 같은 프레임의 공격 판정(TG_PostPhysics)보다 먼저 구간을 엽니다.
	if (const AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetOwner()))
	{
		AddTickPrerequisiteComponent(Character->GetCharacterMovement());
		AddTickPrerequisiteComponent(Character->GetMesh());
		if (UMotionWarpingComponent* Warping = Character->GetMotionWarping())
		{
			Warping->OnPreUpdate.AddDynamic(this, &ThisClass::HandleWarpingPreUpdate);
		}
	}
}

void UAgMontageTimelineComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	TArray<int32> Ids;
	for (const FPlayback& Playback : Playbacks)
	{
		Ids.Add(Playback.InstanceId);
	}
	for (int32 Id : Ids)
	{
		EndPlayback(Id);
	}
	if (const AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetOwner()))
	{
		if (UMotionWarpingComponent* Warping = Character->GetMotionWarping())
		{
			Warping->OnPreUpdate.RemoveDynamic(this, &ThisClass::HandleWarpingPreUpdate);
		}
	}
	Super::EndPlay(EndPlayReason);
}

const FAgMontageTimeline* UAgMontageTimelineComponent::FindTimeline(const UAnimMontage& Montage)
{
	return UAgAssetManager::Get().GetMontageTimelines().Find(Montage);
}

void UAgMontageTimelineComponent::BeginPlayback(int32 InstanceId)
{
	UAnimInstance* Anim = GetAnimInstance();
	const FAnimMontageInstance* Instance = Anim ? Anim->GetMontageInstanceForID(InstanceId) : nullptr;
	if (Instance && !FindPlayback(InstanceId) && StartTracking(*Instance))
	{
		Evaluate(InstanceId, Instance->GetPosition(), EMode::Start);
	}
}

void UAgMontageTimelineComponent::Resync(int32 InstanceId)
{
	UAnimInstance* Anim = GetAnimInstance();
	const FAnimMontageInstance* Instance = Anim ? Anim->GetMontageInstanceForID(InstanceId) : nullptr;
	if (Instance && FindPlayback(InstanceId))
	{
		Evaluate(InstanceId, Instance->GetPosition(), EMode::Jump);
	}
}

void UAgMontageTimelineComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 알림이 재생을 바꿀 수 있으므로 판정할 재생을 먼저 모읍니다.
	UAnimInstance* Anim = GetAnimInstance();
	TArray<int32> Ids;
	for (const FPlayback& Playback : Playbacks)
	{
		Ids.Add(Playback.InstanceId);
	}
	for (int32 Id : Ids)
	{
		const FAnimMontageInstance* Instance = Anim ? Anim->GetMontageInstanceForID(Id) : nullptr;
		if (Instance)
		{
			Evaluate(Id, Instance->GetPosition(), EMode::Advance);
		}
		else
		{
			EndPlayback(Id);
		}
	}
}

UAnimInstance* UAgMontageTimelineComponent::GetAnimInstance() const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	return Character && Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
}

UAgMontageTimelineComponent::FPlayback* UAgMontageTimelineComponent::FindPlayback(int32 InstanceId)
{
	return Playbacks.FindByPredicate(
	    [InstanceId](const FPlayback& Playback)
	    {
		    return Playback.InstanceId == InstanceId;
	    });
}

bool UAgMontageTimelineComponent::StartTracking(const FAnimMontageInstance& Instance)
{
	const FAgMontageTimeline* Source = Instance.Montage ? FindTimeline(*Instance.Montage) : nullptr;
	if (!Source)
	{
		return false;
	}
	FPlayback& Playback = Playbacks.AddDefaulted_GetRef();
	Playback.InstanceId = Instance.GetInstanceID();
	Playback.Timeline = MakeShared<const FAgMontageTimeline>(*Source);
	Playback.Position = Instance.GetPosition();
	const FAgMontageTimeline& Timeline = *Playback.Timeline;
	const TFunction<void(EAgTimelineFeature, int32, float, float)> AddWindow =
	    [&Playback](EAgTimelineFeature Feature, int32 Index, float Start, float End)
	{
		FWindow& Window = Playback.Windows.AddDefaulted_GetRef();
		Window.Feature = Feature;
		Window.Index = Index;
		Window.Start = Start;
		Window.End = End;
	};
	const TFunction<void(EAgTimelineFeature, int32, float)> AddPoint = [&Playback](EAgTimelineFeature Feature, int32 Index, float Time)
	{
		FPoint& Point = Playback.Points.AddDefaulted_GetRef();
		Point.Feature = Feature;
		Point.Index = Index;
		Point.Time = Time;
	};
	for (int32 Index = 0; Index < Timeline.Attack.Num(); ++Index)
	{
		AddWindow(EAgTimelineFeature::Attack, Index, Timeline.Attack[Index].Start, Timeline.Attack[Index].End);
	}
	for (int32 Index = 0; Index < Timeline.HoldTag.Num(); ++Index)
	{
		AddWindow(EAgTimelineFeature::HoldTag, Index, Timeline.HoldTag[Index].Start, Timeline.HoldTag[Index].End);
	}
	for (int32 Index = 0; Index < Timeline.SetCollisionResponseToChannel.Num(); ++Index)
	{
		const FAgTimelineCollisionResponse& Window = Timeline.SetCollisionResponseToChannel[Index];
		AddWindow(EAgTimelineFeature::SetCollisionResponseToChannel, Index, Window.Start, Window.End);
	}
	for (int32 Index = 0; Index < Timeline.Dash.Num(); ++Index)
	{
		AddWindow(EAgTimelineFeature::Dash, Index, Timeline.Dash[Index].Start, Timeline.Dash[Index].End);
	}
	for (int32 Index = 0; Index < Timeline.GameplayEvent.Num(); ++Index)
	{
		AddPoint(EAgTimelineFeature::GameplayEvent, Index, Timeline.GameplayEvent[Index].Time);
	}
	for (int32 Index = 0; Index < Timeline.SpawnProjectile.Num(); ++Index)
	{
		AddPoint(EAgTimelineFeature::SpawnProjectile, Index, Timeline.SpawnProjectile[Index].Time);
	}
	return true;
}

void UAgMontageTimelineComponent::Evaluate(int32 InstanceId, float NewPosition, EMode Mode)
{
	FPlayback* Playback = FindPlayback(InstanceId);
	if (!Playback)
	{
		return;
	}

	struct FPending
	{
		float Time = 0.f;
		EAgTimelinePhase Phase = EAgTimelinePhase::Point;
		int32 Order = 0;
		int32 WindowSlot = INDEX_NONE;
		EAgTimelineFeature Feature = EAgTimelineFeature::Attack;
		int32 Index = INDEX_NONE;
	};
	TArray<FPending> Pending;
	const TFunction<void(float, EAgTimelinePhase, int32, int32, EAgTimelineFeature, int32)> Add =
	    [&Pending](float Time, EAgTimelinePhase Phase, int32 Order, int32 WindowSlot, EAgTimelineFeature Feature, int32 Index)
	{
		FPending& Item = Pending.AddDefaulted_GetRef();
		Item.Time = Time;
		Item.Phase = Phase;
		Item.Order = Order;
		Item.WindowSlot = WindowSlot;
		Item.Feature = Feature;
		Item.Index = Index;
	};

	// 앞으로 진행하면 지나간 구간 경계와 지점을 모두 실행합니다. 재생 시작, 섹션 이동, 반복으로 위치가 뒤로 간 경우는
	// 위치로만 구간을 맞추고, 시작과 섹션 이동에서는 그 위치의 지점만 실행합니다.
	const float Previous = Playback->Position;
	const bool bForward = Mode == EMode::Advance && NewPosition >= Previous;
	for (int32 Slot = 0; Slot < Playback->Windows.Num(); ++Slot)
	{
		const FWindow& Window = Playback->Windows[Slot];
		if (bForward && !Window.bActive && Previous < Window.Start && Window.Start <= NewPosition)
		{
			// 한 번의 판정에서 구간 전체를 지나면 열고 바로 닫습니다.
			Add(Window.Start, EAgTimelinePhase::Begin, Slot, Slot, Window.Feature, Window.Index);
			if (Window.End <= NewPosition)
			{
				Add(Window.End, EAgTimelinePhase::End, Slot, Slot, Window.Feature, Window.Index);
			}
			continue;
		}
		if (bForward && Window.bActive && Window.End <= NewPosition)
		{
			Add(Window.End, EAgTimelinePhase::End, Slot, Slot, Window.Feature, Window.Index);
			continue;
		}
		const bool bInside = Window.Start <= NewPosition && NewPosition < Window.End;
		if (Window.bActive != bInside)
		{
			Add(NewPosition, bInside ? EAgTimelinePhase::Begin : EAgTimelinePhase::End, Slot, Slot, Window.Feature, Window.Index);
		}
	}
	for (int32 Index = 0; Index < Playback->Points.Num(); ++Index)
	{
		const FPoint& Point = Playback->Points[Index];
		const bool bReached = bForward ? (Previous < Point.Time && Point.Time <= NewPosition)
		                               : (Mode != EMode::Advance && FMath::IsNearlyEqual(Point.Time, NewPosition, PointTolerance));
		if (bReached)
		{
			Add(Point.Time, EAgTimelinePhase::Point, Playback->Windows.Num() + Index, INDEX_NONE, Point.Feature, Point.Index);
		}
	}
	Playback->Position = NewPosition;
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
		    return A.Order < B.Order;
	    });

	const TSharedPtr<const FAgMontageTimeline> Timeline = Playback->Timeline;
	for (const FPending& Item : Pending)
	{
		// 알림이 재생을 끝내거나 다른 재생을 더할 수 있으므로 매번 다시 찾습니다. 알림 안에서 섹션을 옮겨 위치가 바뀌었으면
		// 옮긴 위치의 판정이 이미 끝났으므로 남은 항목은 실행하지 않습니다.
		FPlayback* Current = FindPlayback(InstanceId);
		if (!Current || Current->Position != NewPosition)
		{
			return;
		}
		if (Item.WindowSlot != INDEX_NONE)
		{
			Current->Windows[Item.WindowSlot].bActive = Item.Phase == EAgTimelinePhase::Begin;
		}
		FAgTimelineEvent Event;
		Event.InstanceId = InstanceId;
		Event.Feature = Item.Feature;
		Event.Phase = Item.Phase;
		Event.Index = Item.Index;
		Event.Timeline = Timeline.Get();
		OnTimelineEvent.Broadcast(Event);
	}
}

void UAgMontageTimelineComponent::EndPlayback(int32 InstanceId)
{
	FPlayback* Playback = FindPlayback(InstanceId);
	if (!Playback)
	{
		return;
	}
	const TSharedPtr<const FAgMontageTimeline> Timeline = Playback->Timeline;
	TArray<FWindow> Active;
	for (const FWindow& Window : Playback->Windows)
	{
		if (Window.bActive)
		{
			Active.Add(Window);
		}
	}
	Playbacks.RemoveAll(
	    [InstanceId](const FPlayback& Other)
	    {
		    return Other.InstanceId == InstanceId;
	    });
	for (const FWindow& Window : Active)
	{
		FAgTimelineEvent Event;
		Event.InstanceId = InstanceId;
		Event.Feature = Window.Feature;
		Event.Phase = EAgTimelinePhase::End;
		Event.Index = Window.Index;
		Event.Timeline = Timeline.Get();
		OnTimelineEvent.Broadcast(Event);
	}
}

void UAgMontageTimelineComponent::HandleWarpingPreUpdate(UMotionWarpingComponent* Warping)
{
	// 엔진이 워핑 노티파이 구간에 Modifier를 더하는 때와 조건(앞 위치가 구간 안이고 같은 구간의 Modifier가 없음)을 따릅니다.
	// 켜고 지우는 것과 루트 모션 보정은 엔진이 재생 위치로 합니다.
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const FAnimMontageInstance* Instance = Character ? Character->GetRootMotionAnimMontageInstance() : nullptr;
	const FPlayback* Playback = Instance ? FindPlayback(Instance->GetInstanceID()) : nullptr;
	if (!Warping || !Playback)
	{
		return;
	}
	const UAnimMontage* Montage = Instance->Montage.Get();
	const float Previous = Instance->GetPreviousPosition();
	for (const FAgTimelineMotionWarping& Window : Playback->Timeline->MotionWarping)
	{
		if (Window.Start <= Previous && Previous < Window.End && !Warping->ContainsModifier(Montage, Window.Start, Window.End))
		{
			UAgRootMotionModifier_MotionWarping* Modifier = NewObject<UAgRootMotionModifier_MotionWarping>(Warping);
			Modifier->Animation = Montage;
			Modifier->StartTime = Window.Start;
			Modifier->EndTime = Window.End;
			Modifier->SetTimelineSettings(Window.Params, Window.RotationMethod);
			Warping->AddModifier(Modifier);
		}
	}
}
