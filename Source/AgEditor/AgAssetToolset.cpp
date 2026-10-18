// Copyright Woogle. All Rights Reserved.

#include "AgAssetToolset.h"

#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/Skeleton.h"
#include "Animation/WidgetAnimation.h"
#include "AssetToolsModule.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Blueprint.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "IAssetTools.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Logging/TokenizedMessage.h"
#include "Misc/PackageName.h"
#include "Misc/StringOutputDevice.h"
#include "MovieScene.h"
#include "Sections/MovieSceneFloatSection.h"
#include "StateTree.h"
#include "StateTreeCompilerLog.h"
#include "StateTreeConditionBase.h"
#include "StateTreeEditingSubsystem.h"
#include "StateTreeEditorData.h"
#include "StateTreeFactory.h"
#include "StateTreeSchema.h"
#include "StateTreeState.h"
#include "StateTreeTaskBase.h"
#include "Tracks/MovieSceneFloatTrack.h"
#include "UMGEditorProjectSettings.h"
#include "WidgetBlueprint.h"

namespace
{
	UStateTreeEditorData* GetStateTreeEditorData(UStateTree* StateTree)
	{
		return StateTree ? Cast<UStateTreeEditorData>(StateTree->EditorData) : nullptr;
	}

	/** 이름을 '/'로 이은 경로(예: Root/전투)의 상태 */
	UStateTreeState* FindStateTreeState(UStateTree* StateTree, const FString& Path)
	{
		UStateTreeEditorData* EditorData = GetStateTreeEditorData(StateTree);
		TArray<FString> Names;
		Path.ParseIntoArray(Names, TEXT("/"));
		if (!EditorData || Names.IsEmpty())
		{
			return nullptr;
		}

		const TArray<TObjectPtr<UStateTreeState>>* Level = &EditorData->SubTrees;
		UStateTreeState* Found = nullptr;
		for (const FString& Name : Names)
		{
			const TObjectPtr<UStateTreeState>* Match = Level->FindByPredicate([&Name](const UStateTreeState* State)
			{
				return State && State->Name == FName(*Name);
			});
			if (!Match)
			{
				return nullptr;
			}
			Found = *Match;
			Level = &Found->Children;
		}
		return Found;
	}

	/** 노드의 인스턴스 데이터에 언리얼 텍스트 형식의 값을 넣습니다. 값이 비어 있으면 기본값 그대로 둡니다. */
	bool ImportStateTreeNodeValues(FStateTreeEditorNode& Node, const FString& Values)
	{
		if (Values.IsEmpty())
		{
			return true;
		}
		const UScriptStruct* InstanceStruct = Node.Instance.GetScriptStruct();
		if (!InstanceStruct)
		{
			return false;
		}
		FStringOutputDevice Errors;
		const TCHAR* End = InstanceStruct->ImportText(*Values, Node.Instance.GetMutableMemory(), nullptr, PPF_None, &Errors, InstanceStruct->GetName());
		return End && Errors.IsEmpty();
	}

	/** 스키마가 이 노드 구조체를 허용하고, 구조체가 Base의 자식인지 */
	bool IsStateTreeNodeAllowed(UStateTree* StateTree, const UScriptStruct* NodeStruct, const UScriptStruct* Base)
	{
		const UStateTreeEditorData* EditorData = GetStateTreeEditorData(StateTree);
		return NodeStruct && NodeStruct->IsChildOf(Base) && EditorData && EditorData->Schema && EditorData->Schema->IsStructAllowed(NodeStruct);
	}
} // namespace

bool UAgAssetToolset::SetComponentParentSocket(UBlueprint* Blueprint, FName ComponentName, FName SocketName)
{
	USCS_Node* Node = Blueprint && Blueprint->SimpleConstructionScript ? Blueprint->SimpleConstructionScript->FindSCSNode(ComponentName) : nullptr;
	if (!Node)
	{
		return false;
	}

	// 에디터의 Parent Socket 선택과 같은 처리입니다.
	Node->Modify();
	Node->AttachToName = SocketName;
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	return true;
}

bool UAgAssetToolset::SetSlotGroup(USkeleton* Skeleton, FName SlotName, FName GroupName)
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

bool UAgAssetToolset::SetMontageSegments(UAnimMontage* Montage, FName SlotName, const TArray<FAgMontageSegment>& Segments)
{
	// 입력을 모두 검사한 뒤에만 몽타주를 고칩니다. 도중에 실패해 반쯤 바뀐 몽타주가 더티로 남지 않게 합니다.
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
		AnimSegment.SetAnimReference(Segment.Animation, /*bInitialize*/ true);
		AnimSegment.AnimStartTime = Segment.StartTime;
		AnimSegment.AnimEndTime = Segment.EndTime > 0.f ? Segment.EndTime : Segment.Animation->GetPlayLength();
		AnimSegment.AnimPlayRate = FMath::Max(Segment.PlayRate, UE_KINDA_SMALL_NUMBER);
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

bool UAgAssetToolset::SetMontageSections(UAnimMontage* Montage, const TArray<FAgMontageSection>& Sections)
{
	if (!Montage || Sections.IsEmpty())
	{
		return false;
	}

	// 입력을 모두 검사한 뒤에만 몽타주를 고칩니다. 이름이 None이면 엔진이 이름을 지어 붙이므로 실패로 봅니다.
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

	Montage->Modify();
	Montage->CompositeSections.Reset();
	for (const FAgMontageSection& Section : Sections)
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

bool UAgAssetToolset::SetFadeAnimation(UWidgetBlueprint* WidgetBlueprint, FName AnimationName, FName WidgetName, float Duration, float StartOpacity, float EndOpacity)
{
	UWidget* Widget = (WidgetBlueprint && WidgetBlueprint->WidgetTree) ? WidgetBlueprint->WidgetTree->FindWidget(WidgetName) : nullptr;
	if (!Widget || AnimationName.IsNone() || Duration <= 0.f)
	{
		return false;
	}

	WidgetBlueprint->Modify();
	RemoveWidgetAnimation(WidgetBlueprint, AnimationName);

	// 위젯 디자이너에서 새로 만든 애니메이션과 같이 설정합니다.
	UWidgetAnimation* Animation = NewObject<UWidgetAnimation>(WidgetBlueprint, AnimationName, RF_Transactional);
	Animation->SetDisplayLabel(AnimationName.ToString());
	UMovieScene* MovieScene = NewObject<UMovieScene>(Animation, AnimationName, RF_Transactional);
	Animation->MovieScene = MovieScene;
	MovieScene->SetDisplayRate(FFrameRate(GetDefault<UUMGEditorProjectSettings>()->DefaultWidgetAnimationFrameRate, 1));
	const FFrameNumber EndFrame = MovieScene->GetTickResolution().AsFrameNumber(Duration);
	MovieScene->SetPlaybackRange(TRange<FFrameNumber>(FFrameNumber(0), EndFrame + 1));
	MovieScene->GetEditorData().WorkStart = 0.f;
	MovieScene->GetEditorData().WorkEnd = Duration;

	// 애니메이션할 위젯. 재생할 때 이름으로 찾습니다.
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

bool UAgAssetToolset::RemoveWidgetAnimation(UWidgetBlueprint* WidgetBlueprint, FName AnimationName)
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

UStateTree* UAgAssetToolset::CreateStateTree(const FString& AssetPath, TSubclassOf<UStateTreeSchema> Schema)
{
	if (!Schema || !FPackageName::IsValidLongPackageName(AssetPath) || FPackageName::DoesPackageExist(AssetPath) || FindPackage(nullptr, *AssetPath))
	{
		return nullptr;
	}

	// 에디터의 새 StateTree 만들기와 같은 팩토리: 에디터 데이터, 스키마, Root 상태를 만들고 컴파일합니다.
	UStateTreeFactory* Factory = NewObject<UStateTreeFactory>();
	Factory->SetSchemaClass(TObjectPtr<UClass>(Schema.Get()));
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	return Cast<UStateTree>(AssetTools.CreateAsset(FPackageName::GetLongPackageAssetName(AssetPath), FPackageName::GetLongPackagePath(AssetPath), UStateTree::StaticClass(), Factory));
}

bool UAgAssetToolset::AddStateTreeState(UStateTree* StateTree, const FString& ParentPath, FName Name, FName SelectionBehavior)
{
	UStateTreeState* Parent = FindStateTreeState(StateTree, ParentPath);
	const int64 Selection = StaticEnum<EStateTreeStateSelectionBehavior>()->GetValueByNameString(SelectionBehavior.ToString());
	if (!Parent || Name.IsNone() || Selection == INDEX_NONE)
	{
		return false;
	}
	if (Parent->Children.ContainsByPredicate([Name](const UStateTreeState* Child)
	{ return Child && Child->Name == Name; }))
	{
		return false;
	}

	Parent->Modify();
	UStateTreeState& State = Parent->AddChildState(Name);
	State.SelectionBehavior = static_cast<EStateTreeStateSelectionBehavior>(Selection);
	UStateTreeEditingSubsystem::MarkAsModified(StateTree);
	return true;
}

bool UAgAssetToolset::RemoveStateTreeState(UStateTree* StateTree, const FString& StatePath)
{
	UStateTreeState* State = FindStateTreeState(StateTree, StatePath);
	if (!State)
	{
		return false;
	}

	if (UStateTreeState* Parent = State->Parent)
	{
		Parent->Modify();
		Parent->Children.Remove(State);
	}
	else
	{
		UStateTreeEditorData* EditorData = GetStateTreeEditorData(StateTree);
		EditorData->Modify();
		EditorData->SubTrees.Remove(State);
	}
	UStateTreeEditingSubsystem::MarkAsModified(StateTree);
	return true;
}

bool UAgAssetToolset::AddStateTreeTask(UStateTree* StateTree, const FString& StatePath, UScriptStruct* Task, const FString& InstanceValues)
{
	UStateTreeState* State = FindStateTreeState(StateTree, StatePath);
	if (!State || !IsStateTreeNodeAllowed(StateTree, Task, FStateTreeTaskBase::StaticStruct()))
	{
		return false;
	}

	State->Modify();
	FStateTreeEditorNode& Node = State->Tasks.AddDefaulted_GetRef();
	Node.InitializeAs(State, Task);
	if (!ImportStateTreeNodeValues(Node, InstanceValues))
	{
		State->Tasks.Pop();
		return false;
	}
	UStateTreeEditingSubsystem::MarkAsModified(StateTree);
	return true;
}

bool UAgAssetToolset::ClearStateTreeTasks(UStateTree* StateTree, const FString& StatePath)
{
	UStateTreeState* State = FindStateTreeState(StateTree, StatePath);
	if (!State)
	{
		return false;
	}

	State->Modify();
	State->Tasks.Reset();
	UStateTreeEditingSubsystem::MarkAsModified(StateTree);
	return true;
}

bool UAgAssetToolset::AddStateTreeTransition(UStateTree* StateTree, const FString& StatePath, FName Trigger, FGameplayTag EventTag, const FString& TargetPath, EStateTreeTransitionPriority Priority, UScriptStruct* Condition, const FString& ConditionValues)
{
	UStateTreeState* State = FindStateTreeState(StateTree, StatePath);
	const UStateTreeState* Target = FindStateTreeState(StateTree, TargetPath);
	const int64 TriggerValue = StaticEnum<EStateTreeTransitionTrigger>()->GetValueByNameString(Trigger.ToString());
	if (!State || !Target || TriggerValue == INDEX_NONE || TriggerValue == static_cast<int64>(EStateTreeTransitionTrigger::None))
	{
		return false;
	}
	const EStateTreeTransitionTrigger TriggerType = static_cast<EStateTreeTransitionTrigger>(TriggerValue);
	if ((TriggerType == EStateTreeTransitionTrigger::OnEvent && !EventTag.IsValid()) || (Condition && !IsStateTreeNodeAllowed(StateTree, Condition, FStateTreeConditionBase::StaticStruct())))
	{
		return false;
	}

	State->Modify();
	FStateTreeTransition& Transition = TriggerType == EStateTreeTransitionTrigger::OnEvent
	                                       ? State->AddTransition(TriggerType, EventTag, EStateTreeTransitionType::GotoState, Target)
	                                       : State->AddTransition(TriggerType, EStateTreeTransitionType::GotoState, Target);
	Transition.Priority = Priority;
	if (Condition)
	{
		FStateTreeEditorNode& Node = Transition.Conditions.AddDefaulted_GetRef();
		Node.InitializeAs(State, Condition);
		if (!ImportStateTreeNodeValues(Node, ConditionValues))
		{
			State->Transitions.Pop();
			return false;
		}
	}
	UStateTreeEditingSubsystem::MarkAsModified(StateTree);
	return true;
}

bool UAgAssetToolset::ClearStateTreeTransitions(UStateTree* StateTree, const FString& StatePath)
{
	UStateTreeState* State = FindStateTreeState(StateTree, StatePath);
	if (!State)
	{
		return false;
	}

	State->Modify();
	State->Transitions.Reset();
	UStateTreeEditingSubsystem::MarkAsModified(StateTree);
	return true;
}

FString UAgAssetToolset::CompileStateTree(UStateTree* StateTree)
{
	if (!GetStateTreeEditorData(StateTree))
	{
		return TEXT("Failed\nNo StateTree editor data");
	}

	FStateTreeCompilerLog Log;
	TArray<FString> Lines;
	if (!UStateTreeEditingSubsystem::CompileStateTree(StateTree, Log))
	{
		Lines.Add(TEXT("Failed"));
	}
	for (const TSharedRef<FTokenizedMessage>& Message : Log.ToTokenizedMessages())
	{
		Lines.Add(Message->ToText().ToString());
	}
	StateTree->MarkPackageDirty();
	return FString::Join(Lines, TEXT("\n"));
}
