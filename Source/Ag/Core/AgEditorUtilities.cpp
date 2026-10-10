// Copyright Woogle. All Rights Reserved.

#include "Core/AgEditorUtilities.h"

#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/Skeleton.h"

#if WITH_EDITOR

bool UAgEditorUtilities::SetSlotGroup(USkeleton* Skeleton, FName SlotName, FName GroupName)
{
	if (!Skeleton || SlotName.IsNone() || GroupName.IsNone())
	{
		return false;
	}

	Skeleton->Modify();
	Skeleton->AddSlotGroupName(GroupName);
	Skeleton->RegisterSlotNode(SlotName);
	Skeleton->SetSlotGroupName(SlotName, GroupName);
	Skeleton->MarkPackageDirty();
	return Skeleton->GetSlotGroupName(SlotName) == GroupName;
}

bool UAgEditorUtilities::SetMontageSegments(UAnimMontage* Montage, FName SlotName, const TArray<FAgMontageSegment>& Segments)
{
	if (!Montage || Montage->SlotAnimTracks.IsEmpty() || Segments.IsEmpty())
	{
		return false;
	}

	Montage->Modify();
	FSlotAnimationTrack& SlotTrack = Montage->SlotAnimTracks[0];
	SlotTrack.SlotName = SlotName;
	SlotTrack.AnimTrack.AnimSegments.Reset();

	float Position = 0.f;
	for (const FAgMontageSegment& Segment : Segments)
	{
		if (!Segment.Animation)
		{
			return false;
		}
		FAnimSegment& AnimSegment = SlotTrack.AnimTrack.AnimSegments.AddDefaulted_GetRef();
		AnimSegment.SetAnimReference(Segment.Animation, /*bInitialize*/ true);
		AnimSegment.AnimStartTime = Segment.StartTime;
		AnimSegment.AnimEndTime = Segment.EndTime > 0.f ? Segment.EndTime : Segment.Animation->GetPlayLength();
		AnimSegment.AnimPlayRate = 1.f;
		AnimSegment.LoopingCount = 1;
		AnimSegment.StartPos = Position;
		Position += AnimSegment.GetLength();
	}

	Montage->SetCompositeLength(Position);
	for (FCompositeSection& Section : Montage->CompositeSections)
	{
		if (Section.GetTime() > Position)
		{
			Section.SetTime(0.f);
		}
	}
	Montage->PostEditChange();
	Montage->MarkPackageDirty();
	return true;
}

#endif
