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
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraSystem.h"
#include "NiagaraTypes.h"
#include "NiagaraUserRedirectionParameterStore.h"
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
#include "UObject/StructOnScope.h"
#include "ViewModels/NiagaraEmitterHandleViewModel.h"
#include "ViewModels/NiagaraSystemViewModel.h"
#include "ViewModels/Stack/NiagaraStackFunctionInput.h"
#include "ViewModels/Stack/NiagaraStackModuleItem.h"
#include "ViewModels/Stack/NiagaraStackViewModel.h"
#include "WidgetBlueprint.h"

namespace
{
	UStateTreeEditorData* GetStateTreeEditorData(UStateTree* StateTree)
	{
		return StateTree ? Cast<UStateTreeEditorData>(StateTree->EditorData) : nullptr;
	}

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

	bool IsStateTreeNodeAllowed(UStateTree* StateTree, const UScriptStruct* NodeStruct, const UScriptStruct* Base)
	{
		const UStateTreeEditorData* EditorData = GetStateTreeEditorData(StateTree);
		return NodeStruct && NodeStruct->IsChildOf(Base) && EditorData && EditorData->Schema && EditorData->Schema->IsStructAllowed(NodeStruct);
	}

	template <typename TValue>
	bool SetNiagaraUserParameter(UNiagaraSystem* System, FName Name, const FNiagaraTypeDefinition& Type, const TValue& Value)
	{
		if (!System || Name.IsNone())
		{
			return false;
		}

		FNiagaraVariable Variable(Type, Name);
		FNiagaraUserRedirectionParameterStore::MakeUserVariable(Variable);
		FNiagaraUserRedirectionParameterStore& Parameters = System->GetExposedParameters();
		const bool bOtherType = Parameters.ReadParameterVariables().ContainsByPredicate([&Variable](const FNiagaraVariableWithOffset& Existing)
		{
			return Existing.GetName() == Variable.GetName() && Existing.GetType() != Variable.GetType();
		});
		if (bOtherType)
		{
			return false;
		}

		System->Modify();
		return Parameters.SetParameterValue(Value, Variable, true);
	}

	TSharedRef<FNiagaraSystemViewModel> MakeNiagaraSystemViewModel(UNiagaraSystem& System)
	{
		// 편집기 창 없이 스택만 쓰도록 엔진의 데이터 처리용 뷰모델과 같은 옵션으로 만듭니다.
		TSharedRef<FNiagaraSystemViewModel> ViewModel = MakeShared<FNiagaraSystemViewModel>();
		FNiagaraSystemViewModelOptions Options;
		Options.bCanAutoCompile = false;
		Options.bCanModifyEmittersFromTimeline = false;
		Options.bCanSimulate = false;
		Options.bCompileForEdit = false;
		Options.bIsForDataProcessingOnly = true;
		Options.EditMode = ENiagaraSystemViewModelEditMode::SystemAsset;
		Options.MessageLogGuid = System.GetAssetGuid();
		ViewModel->Initialize(System, Options);
		return ViewModel;
	}

	void CompileNiagaraSystem(UNiagaraSystem& System)
	{
		System.RequestCompile(false);
		System.WaitForCompilationComplete();
		System.MarkPackageDirty();
	}

	TArray<UNiagaraStackModuleItem*> GetNiagaraModules(const FNiagaraSystemViewModel& ViewModel, FName EmitterName)
	{
		TArray<UNiagaraStackModuleItem*> Modules;
		for (const TSharedRef<FNiagaraEmitterHandleViewModel>& Emitter : ViewModel.GetEmitterHandleViewModels())
		{
			UNiagaraStackViewModel* Stack = Emitter->GetName() == EmitterName ? Emitter->GetEmitterStackViewModel() : nullptr;
			if (Stack && Stack->GetRootEntry())
			{
				Stack->GetRootEntry()->GetUnfilteredChildrenOfType(Modules, true);
			}
		}
		return Modules;
	}

	TArray<UNiagaraStackFunctionInput*> GetDirectInputs(UNiagaraStackModuleItem& Module)
	{
		TArray<UNiagaraStackFunctionInput*> Inputs;
		Module.GetUnfilteredChildrenOfType(Inputs, true);
		Inputs.RemoveAll([&Module](const UNiagaraStackFunctionInput* Input)
		{ return Input->GetInputFunctionCallNodePtr() != &Module.GetModuleNode(); });
		return Inputs;
	}

	FString DescribeNiagaraInput(UNiagaraStackFunctionInput& Input)
	{
		using EValueMode = UNiagaraStackFunctionInput::EValueMode;
		switch (Input.GetValueMode())
		{
		case EValueMode::Local:
			if (const TSharedPtr<const FStructOnScope> Local = Input.GetLocalValueStruct())
			{
				if (const UEnum* Enum = Input.GetInputType().GetEnum())
				{
					return Enum->GetDisplayNameTextByValue(reinterpret_cast<const FNiagaraInt32*>(Local->GetStructMemory())->Value).ToString();
				}
				FString Text;
				CastChecked<const UScriptStruct>(Local->GetStruct())->ExportText(Text, Local->GetStructMemory(), nullptr, nullptr, PPF_None, nullptr);
				return Text;
			}
			return FString();
		case EValueMode::Linked:
		case EValueMode::LinkedShared:
			return Input.GetLinkedParameterValue().GetName().ToString();
		case EValueMode::Dynamic:
			return FString::Printf(TEXT("Dynamic %s"), Input.GetDynamicInputNode() ? *Input.GetDynamicInputNode()->GetFunctionName() : TEXT(""));
		case EValueMode::Expression:
			return FString::Printf(TEXT("Expression %s"), *Input.GetCustomExpressionText().ToString());
		default:
			return TEXT("-");
		}
	}

	TSharedPtr<FStructOnScope> MakeNiagaraLocalValue(const FNiagaraTypeDefinition& Type, const FString& Value)
	{
		const UScriptStruct* Struct = Type.GetScriptStruct();
		if (!Struct)
		{
			return nullptr;
		}

		TSharedRef<FStructOnScope> Local = MakeShared<FStructOnScope>(Struct);
		uint8* Memory = Local->GetStructMemory();
		if (const UEnum* Enum = Type.GetEnum())
		{
			for (int32 Index = 0; Index < Enum->NumEnums(); ++Index)
			{
				if (Enum->GetDisplayNameTextByIndex(Index).ToString() == Value || Enum->GetNameStringByIndex(Index) == Value)
				{
					reinterpret_cast<FNiagaraInt32*>(Memory)->Value = static_cast<int32>(Enum->GetValueByIndex(Index));
					return Local;
				}
			}
			return nullptr;
		}
		if (Type.IsSameBaseDefinition(FNiagaraTypeDefinition::GetBoolDef()))
		{
			reinterpret_cast<FNiagaraBool*>(Memory)->SetValue(Value.ToBool());
			return Local;
		}

		FStringOutputDevice Errors;
		const FProperty* SingleProperty = Struct->PropertyLink && !Struct->PropertyLink->PropertyLinkNext ? Struct->PropertyLink : nullptr;
		const TCHAR* End = SingleProperty && !Value.StartsWith(TEXT("("))
			                   ? SingleProperty->ImportText_Direct(*Value, SingleProperty->ContainerPtrToValuePtr<void>(Memory), nullptr, PPF_None, &Errors)
			                   : Struct->ImportText(*Value, Memory, nullptr, PPF_None, &Errors, Struct->GetName());
		return End && Errors.IsEmpty() ? Local.ToSharedPtr() : nullptr;
	}
} // namespace

bool UAgAssetToolset::SetComponentParentSocket(UBlueprint* Blueprint, FName ComponentName, FName SocketName)
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

bool UAgAssetToolset::SetNiagaraUserColor(UNiagaraSystem* System, FName Name, FLinearColor Value)
{
	return SetNiagaraUserParameter(System, Name, FNiagaraTypeDefinition::GetColorDef(), Value);
}

bool UAgAssetToolset::SetNiagaraUserFloat(UNiagaraSystem* System, FName Name, float Value)
{
	return SetNiagaraUserParameter(System, Name, FNiagaraTypeDefinition::GetFloatDef(), Value);
}

FString UAgAssetToolset::GetNiagaraModuleInputs(UNiagaraSystem* System, FName EmitterName)
{
	if (!System)
	{
		return FString();
	}

	const TSharedRef<FNiagaraSystemViewModel> ViewModel = MakeNiagaraSystemViewModel(*System);
	TArray<FString> Lines;
	for (UNiagaraStackModuleItem* Module : GetNiagaraModules(*ViewModel, EmitterName))
	{
		for (UNiagaraStackFunctionInput* Input : GetDirectInputs(*Module))
		{
			Lines.Add(FString::Printf(TEXT("%s | %s | %s"), *Module->GetDisplayName().ToString(), *Input->GetDisplayName().ToString(), *DescribeNiagaraInput(*Input)));
		}
	}
	return FString::Join(Lines, TEXT("\n"));
}

bool UAgAssetToolset::SetNiagaraModuleInput(UNiagaraSystem* System, FName EmitterName, const FString& ModuleName, const FString& InputName, const FString& Value)
{
	if (!System)
	{
		return false;
	}

	const TSharedRef<FNiagaraSystemViewModel> ViewModel = MakeNiagaraSystemViewModel(*System);
	UNiagaraStackFunctionInput* Input = nullptr;
	for (UNiagaraStackModuleItem* Module : GetNiagaraModules(*ViewModel, EmitterName))
	{
		if (Module->GetDisplayName().ToString() != ModuleName)
		{
			continue;
		}
		for (UNiagaraStackFunctionInput* Candidate : GetDirectInputs(*Module))
		{
			if (Candidate->GetDisplayName().ToString() == InputName)
			{
				Input = Candidate;
			}
		}
	}
	if (!Input)
	{
		return false;
	}

	const bool bLink = Value.StartsWith(TEXT("User.")) || Value.StartsWith(TEXT("Engine.")) || Value.StartsWith(TEXT("Emitter.")) || Value.StartsWith(TEXT("System.")) || Value.StartsWith(TEXT("Particles."));
	if (bLink)
	{
		Input->SetLinkedParameterValue(FNiagaraVariableBase(Input->GetInputType(), FName(*Value)));
	}
	else
	{
		const TSharedPtr<FStructOnScope> Local = MakeNiagaraLocalValue(Input->GetInputType(), Value);
		if (!Local)
		{
			return false;
		}
		Input->SetLocalValue(Local.ToSharedRef());
	}

	CompileNiagaraSystem(*System);
	return true;
}

bool UAgAssetToolset::RemoveNiagaraModule(UNiagaraSystem* System, FName EmitterName, const FString& ModuleName)
{
	if (!System)
	{
		return false;
	}

	const TSharedRef<FNiagaraSystemViewModel> ViewModel = MakeNiagaraSystemViewModel(*System);
	for (UNiagaraStackModuleItem* Module : GetNiagaraModules(*ViewModel, EmitterName))
	{
		FText Message;
		if (Module->GetDisplayName().ToString() == ModuleName && Module->TestCanDeleteWithMessage(Message))
		{
			Module->Delete();
			CompileNiagaraSystem(*System);
			return true;
		}
	}
	return false;
}

bool UAgAssetToolset::RemoveNiagaraUserParameter(UNiagaraSystem* System, FName Name)
{
	if (!System || Name.IsNone())
	{
		return false;
	}

	FNiagaraVariable Target(FNiagaraTypeDefinition::GetFloatDef(), Name);
	FNiagaraUserRedirectionParameterStore::MakeUserVariable(Target);
	const FNiagaraVariableWithOffset* Existing = System->GetExposedParameters().ReadParameterVariables().FindByPredicate([&Target](const FNiagaraVariableWithOffset& Variable)
	{ return Variable.GetName() == Target.GetName(); });
	if (!Existing)
	{
		return false;
	}

	// 엔진 편집기 함수는 메시지 로그 키 없이 뷰모델을 만들어 check에 걸리므로, 참조를 먼저 끊었다는 전제로 저장소에서 바로 뺍니다.
	const FNiagaraVariable Removed(*Existing);
	System->Modify();
	System->GetExposedParameters().RemoveParameter(Removed);
	System->HandleVariableRemoved(Removed, true);
	CompileNiagaraSystem(*System);
	return true;
}

bool UAgAssetToolset::RenameNiagaraEmitter(UNiagaraSystem* System, FName EmitterName, FName NewName)
{
	if (!System || NewName.IsNone())
	{
		return false;
	}

	const TSharedRef<FNiagaraSystemViewModel> ViewModel = MakeNiagaraSystemViewModel(*System);
	for (const TSharedRef<FNiagaraEmitterHandleViewModel>& Emitter : ViewModel->GetEmitterHandleViewModels())
	{
		if (Emitter->GetName() == EmitterName)
		{
			Emitter->SetName(NewName);
			CompileNiagaraSystem(*System);
			return Emitter->GetName() == NewName;
		}
	}
	return false;
}

UStateTree* UAgAssetToolset::CreateStateTree(const FString& AssetPath, TSubclassOf<UStateTreeSchema> Schema)
{
	if (!Schema || !FPackageName::IsValidLongPackageName(AssetPath) || FPackageName::DoesPackageExist(AssetPath) || FindPackage(nullptr, *AssetPath))
	{
		return nullptr;
	}

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
