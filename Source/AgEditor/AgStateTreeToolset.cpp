// Copyright Woogle. All Rights Reserved.

#include "AgStateTreeToolset.h"

#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Logging/TokenizedMessage.h"
#include "Misc/PackageName.h"
#include "Misc/StringOutputDevice.h"
#include "StateTree.h"
#include "StateTreeCompilerLog.h"
#include "StateTreeConditionBase.h"
#include "StateTreeEditingSubsystem.h"
#include "StateTreeEditorData.h"
#include "StateTreeFactory.h"
#include "StateTreeSchema.h"
#include "StateTreeState.h"
#include "StateTreeTaskBase.h"

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
} // namespace

UStateTree* UAgStateTreeToolset::CreateStateTree(const FString& AssetPath, TSubclassOf<UStateTreeSchema> Schema)
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

bool UAgStateTreeToolset::AddStateTreeState(UStateTree* StateTree, const FString& ParentPath, FName Name, FName SelectionBehavior)
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

bool UAgStateTreeToolset::RemoveStateTreeState(UStateTree* StateTree, const FString& StatePath)
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

bool UAgStateTreeToolset::AddStateTreeTask(UStateTree* StateTree, const FString& StatePath, UScriptStruct* Task, const FString& InstanceValues)
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

bool UAgStateTreeToolset::ClearStateTreeTasks(UStateTree* StateTree, const FString& StatePath)
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

bool UAgStateTreeToolset::AddStateTreeTransition(UStateTree* StateTree, const FString& StatePath, FName Trigger, FGameplayTag EventTag, const FString& TargetPath, EStateTreeTransitionPriority Priority, UScriptStruct* Condition, const FString& ConditionValues)
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

bool UAgStateTreeToolset::ClearStateTreeTransitions(UStateTree* StateTree, const FString& StatePath)
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

FString UAgStateTreeToolset::CompileStateTree(UStateTree* StateTree)
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
