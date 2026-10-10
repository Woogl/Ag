// Copyright Woogle. All Rights Reserved.

#include "Core/AgEditorUtilities.h"

#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/Skeleton.h"

#if WITH_EDITOR
#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "MovieScene.h"
#include "Sections/MovieSceneFloatSection.h"
#include "Tracks/MovieSceneFloatTrack.h"
#include "UMGEditorProjectSettings.h"
#include "WidgetBlueprint.h"
#endif

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

bool UAgEditorUtilities::SetMontageSections(UAnimMontage* Montage, const TArray<FAgMontageSection>& Sections)
{
	if (!Montage || Sections.IsEmpty())
	{
		return false;
	}

	Montage->Modify();
	Montage->CompositeSections.Reset();
	for (const FAgMontageSection& Section : Sections)
	{
		if (Montage->AddAnimCompositeSection(Section.Name, Section.StartTime) == INDEX_NONE)
		{
			return false;
		}
	}
	for (const FAgMontageSection& Section : Sections)
	{
		const int32 Index = Montage->GetSectionIndex(Section.Name);
		if (Index == INDEX_NONE)
		{
			return false;
		}
		Montage->CompositeSections[Index].NextSectionName = Section.NextSection;
	}
	Montage->PostEditChange();
	Montage->MarkPackageDirty();
	return true;
}

bool UAgEditorUtilities::SetFadeAnimation(UWidgetBlueprint* WidgetBlueprint, FName AnimationName, FName WidgetName, float Duration, float StartOpacity, float EndOpacity)
{
	UWidget* Widget = (WidgetBlueprint && WidgetBlueprint->WidgetTree) ? WidgetBlueprint->WidgetTree->FindWidget(WidgetName) : nullptr;
	if (!Widget || AnimationName.IsNone() || Duration <= 0.f)
	{
		return false;
	}

	WidgetBlueprint->Modify();

	// An animation of that name is deleted the way the widget designer deletes one (moved out of the way, then forgotten).
	for (int32 Index = WidgetBlueprint->Animations.Num() - 1; Index >= 0; --Index)
	{
		UWidgetAnimation* Existing = WidgetBlueprint->Animations[Index];
		if (Existing && Existing->GetFName() == AnimationName)
		{
			Existing->Rename(nullptr, GetTransientPackage());
			WidgetBlueprint->Animations.RemoveAt(Index);
		}
	}
	WidgetBlueprint->OnVariableRemoved(AnimationName);

	// Set up like a new animation made in the widget designer.
	UWidgetAnimation* Animation = NewObject<UWidgetAnimation>(WidgetBlueprint, AnimationName, RF_Transactional);
	Animation->SetDisplayLabel(AnimationName.ToString());
	UMovieScene* MovieScene = NewObject<UMovieScene>(Animation, AnimationName, RF_Transactional);
	Animation->MovieScene = MovieScene;
	MovieScene->SetDisplayRate(FFrameRate(GetDefault<UUMGEditorProjectSettings>()->DefaultWidgetAnimationFrameRate, 1));
	const FFrameNumber EndFrame = MovieScene->GetTickResolution().AsFrameNumber(Duration);
	MovieScene->SetPlaybackRange(TRange<FFrameNumber>(FFrameNumber(0), EndFrame + 1));
	MovieScene->GetEditorData().WorkStart = 0.f;
	MovieScene->GetEditorData().WorkEnd = Duration;

	// The animated widget, found by name when the animation plays.
	const FGuid Binding = MovieScene->AddPossessable(Widget->GetName(), Widget->GetClass());
	FWidgetAnimationBinding& WidgetBinding = Animation->AnimationBindings.AddDefaulted_GetRef();
	WidgetBinding.WidgetName = Widget->GetFName();
	WidgetBinding.AnimationGuid = Binding;

	UMovieSceneFloatTrack* Track = MovieScene->AddTrack<UMovieSceneFloatTrack>(Binding);
	Track->SetPropertyNameAndPath(TEXT("RenderOpacity"), TEXT("RenderOpacity"));
	UMovieSceneFloatSection* Section = CastChecked<UMovieSceneFloatSection>(Track->CreateNewSection());
	Section->SetRange(TRange<FFrameNumber>(FFrameNumber(0), EndFrame + 1));
	Section->GetChannel().AddLinearKey(FFrameNumber(0), StartOpacity);
	Section->GetChannel().AddLinearKey(EndFrame, EndOpacity);
	Track->AddSection(*Section);

	WidgetBlueprint->Animations.Add(Animation);
	WidgetBlueprint->OnVariableAdded(Animation->GetFName());
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBlueprint);
	return true;
}

#endif
