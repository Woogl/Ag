// Copyright Woogle. All Rights Reserved.

#include "AgAnimationToolset.h"

#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/Skeleton.h"

bool UAgAnimationToolset::SetSlotGroup(USkeleton* Skeleton, FName SlotName, FName GroupName)
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

bool UAgAnimationToolset::SetMontageSegments(UAnimMontage* Montage, FName SlotName, const TArray<FAgMontageSegment>& Segments)
{
	// 입력을 모두 검사한 뒤에만 몽타주를 고칩니다.
	// 도중에 실패해 반쯤 바뀐 몽타주가 더티로 남지 않게 합니다.
	if (!Montage || Montage->SlotAnimTracks.IsEmpty() || Segments.IsEmpty() || Segments.ContainsByPredicate([](const FAgMontageSegment& Segment)
	{ return !Segment.Animation; }))
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
		FAnimSegment& AnimSegment = SlotTrack.AnimTrack.AnimSegments.AddDefaulted_GetRef();
		AnimSegment.SetAnimReference(Segment.Animation, true);
		AnimSegment.AnimStartTime = Segment.StartTime;
		AnimSegment.AnimEndTime = Segment.EndTime > 0.f ? Segment.EndTime : Segment.Animation->GetPlayLength();
		AnimSegment.AnimPlayRate = FMath::Max(Segment.PlayRate, UE_KINDA_SMALL_NUMBER);
		AnimSegment.LoopingCount = 1;
		AnimSegment.StartPos = Position;
		Position += AnimSegment.GetLength();
	}

	Montage->SetCompositeLength(Position);
	// 섹션은 옛 조각의 시퀀스에 링크된 채로 남으므로, 같은 시각으로 새 조각에 다시 링크합니다.
	for (FCompositeSection& Section : Montage->CompositeSections)
	{
		const float Time = Section.GetTime();
		Section.Link(Montage, Time > Position ? 0.f : Time);
	}
	Montage->PostEditChange();
	Montage->MarkPackageDirty();
	return true;
}

bool UAgAnimationToolset::SetMontageSections(UAnimMontage* Montage, const TArray<FAgMontageSection>& Sections)
{
	if (!Montage || Sections.IsEmpty())
	{
		return false;
	}

	// 입력을 모두 검사한 뒤에만 몽타주를 고칩니다.
	// 이름이 None이면 엔진이 이름을 지어 붙이므로 실패로 봅니다.
	TSet<FName> Names;
	for (const FAgMontageSection& Section : Sections)
	{
		bool bRepeated = false;
		Names.Add(Section.Name, &bRepeated);
		if (Section.Name.IsNone() || bRepeated)
		{
			return false;
		}
	}
	if (Sections.ContainsByPredicate([&Names](const FAgMontageSection& Section)
	{ return !Section.NextSection.IsNone() && !Names.Contains(Section.NextSection); }))
	{
		return false;
	}

	// 엔진은 위치로 섹션을 찾을 때 섹션이 시작 시간 순서라고 보므로, 그 순서로 넣습니다.
	TArray<FAgMontageSection> SortedSections = Sections;
	SortedSections.StableSort([](const FAgMontageSection& A, const FAgMontageSection& B)
	{ return A.StartTime < B.StartTime; });
	Montage->Modify();
	Montage->CompositeSections.Reset();
	for (const FAgMontageSection& Section : SortedSections)
	{
		Montage->AddAnimCompositeSection(Section.Name, Section.StartTime);
	}
	for (const FAgMontageSection& Section : Sections)
	{
		const int32 Index = Montage->GetSectionIndex(Section.Name);
		if (Index != INDEX_NONE)
		{
			Montage->CompositeSections[Index].NextSectionName = Section.NextSection;
		}
	}
	Montage->PostEditChange();
	Montage->MarkPackageDirty();
	return true;
}
