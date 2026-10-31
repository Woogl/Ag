// Copyright Woogle. All Rights Reserved.

#include "AgBlueprintToolset.h"

#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Blueprint.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "MovieScene.h"
#include "Sections/MovieSceneFloatSection.h"
#include "Tracks/MovieSceneFloatTrack.h"
#include "UMGEditorProjectSettings.h"
#include "WidgetBlueprint.h"

bool UAgBlueprintToolset::SetComponentParentSocket(UBlueprint* Blueprint, FName ComponentName, FName SocketName)
{
	USCS_Node* Node = Blueprint && Blueprint->SimpleConstructionScript ? Blueprint->SimpleConstructionScript->FindSCSNode(ComponentName) : nullptr;
	if (!Node)
	{
		return false;
	}

	Node->Modify();
	Node->AttachToName = SocketName;
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	return true;
}

bool UAgBlueprintToolset::SetFadeAnimation(UWidgetBlueprint* WidgetBlueprint, FName AnimationName, FName WidgetName, float Duration, float StartOpacity, float EndOpacity)
{
	UWidget* Widget = (WidgetBlueprint && WidgetBlueprint->WidgetTree) ? WidgetBlueprint->WidgetTree->FindWidget(WidgetName) : nullptr;
	if (!Widget || AnimationName.IsNone() || Duration <= 0.f)
	{
		return false;
	}

	WidgetBlueprint->Modify();
	RemoveWidgetAnimation(WidgetBlueprint, AnimationName);

	UWidgetAnimation* Animation = NewObject<UWidgetAnimation>(WidgetBlueprint, AnimationName, RF_Transactional);
	Animation->SetDisplayLabel(AnimationName.ToString());
	UMovieScene* MovieScene = NewObject<UMovieScene>(Animation, AnimationName, RF_Transactional);
	Animation->MovieScene = MovieScene;
	MovieScene->SetDisplayRate(FFrameRate(GetDefault<UUMGEditorProjectSettings>()->DefaultWidgetAnimationFrameRate, 1));
	const FFrameNumber EndFrame = MovieScene->GetTickResolution().AsFrameNumber(Duration);
	MovieScene->SetPlaybackRange(TRange<FFrameNumber>(FFrameNumber(0), EndFrame + 1));
	MovieScene->GetEditorData().WorkStart = 0.f;
	MovieScene->GetEditorData().WorkEnd = Duration;

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

bool UAgBlueprintToolset::RemoveWidgetAnimation(UWidgetBlueprint* WidgetBlueprint, FName AnimationName)
{
	if (!WidgetBlueprint || AnimationName.IsNone())
	{
		return false;
	}

	// 이름을 다시 쓸 수 있도록 임시 패키지로 옮긴 뒤 목록에서 뺍니다.
	WidgetBlueprint->Modify();
	bool bRemoved = false;
	for (int32 Index = WidgetBlueprint->Animations.Num() - 1; Index >= 0; --Index)
	{
		UWidgetAnimation* Existing = WidgetBlueprint->Animations[Index];
		if (Existing && Existing->GetFName() == AnimationName)
		{
			Existing->Rename(nullptr, GetTransientPackage());
			WidgetBlueprint->Animations.RemoveAt(Index);
			bRemoved = true;
		}
	}
	WidgetBlueprint->OnVariableRemoved(AnimationName);
	if (bRemoved)
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBlueprint);
	}
	return bRemoved;
}
