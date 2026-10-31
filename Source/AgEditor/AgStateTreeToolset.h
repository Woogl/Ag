// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "StateTreeTypes.h"
#include "Templates/SubclassOf.h"
#include "ToolsetRegistry/ToolsetDefinition.h"
#include "AgStateTreeToolset.generated.h"

class UStateTree;
class UStateTreeSchema;

/**
 * 고친 에셋은 저장하지 않으므로 에셋 도구로 저장합니다. 상태는 경로로 가리키고(이름을 '/'로 이음, 예: Root/전투),
 * 엔진의 StateTree 조회 도구로 읽습니다.
 */
UCLASS(Hidden)
class UAgStateTreeToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/**
	 * 스키마 클래스로 새 StateTree 에셋을 만듭니다. 'Root' 상태 하나가 들어 있습니다.
	 * 스키마의 속성(AI 스키마의 AIControllerClass, ContextActorClass 등)은 엔진의 ObjectTools.set_properties로
	 * 에디터 데이터의 Schema 하위 객체에 씁니다.
	 * @param AssetPath 예: /Game/AI/ST_Enemy
	 * @return 만든 에셋. 그 경로에 에셋이 이미 있거나 만들지 못하면 null입니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor", meta = (AICallable))
	static UStateTree* CreateStateTree(const FString& AssetPath, TSubclassOf<UStateTreeSchema> Schema);

	/**
	 * ParentPath 상태의 마지막 자식으로 상태를 더합니다.
	 * @param SelectionBehavior 자식 상태를 고르는 방식(EStateTreeStateSelectionBehavior 값 이름: TryEnterState,
	 *        TrySelectChildrenInOrder 등). 자식이 없는 상태는 TryEnterState입니다.
	 * @return 부모가 없거나, 같은 이름의 형제가 있거나, SelectionBehavior를 모르면 false입니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor", meta = (AICallable))
	static bool AddStateTreeState(UStateTree* StateTree, const FString& ParentPath, FName Name, FName SelectionBehavior);

	/** 상태를 자식 상태와 함께 지웁니다. 그 상태로 가던 전환은 남으므로 컴파일 오류로 확인합니다. */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor", meta = (AICallable))
	static bool RemoveStateTreeState(UStateTree* StateTree, const FString& StatePath);

	/**
	 * 상태에 태스크를 더합니다.
	 * @param Task 태스크 구조체(예: /Script/Ag.AgEnemyAITask_RunPattern). 스키마가 허용하는 태스크여야 합니다.
	 * @param InstanceValues 태스크 인스턴스 데이터의 값. 언리얼 텍스트 형식이고(예: (bResponse=True)), 비우면 기본값입니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor", meta = (AICallable))
	static bool AddStateTreeTask(UStateTree* StateTree, const FString& StatePath, UScriptStruct* Task, const FString& InstanceValues);

	/** 상태의 태스크를 모두 지웁니다. */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor", meta = (AICallable))
	static bool ClearStateTreeTasks(UStateTree* StateTree, const FString& StatePath);

	/**
	 * 상태에 TargetPath 상태로 가는 전환을 더합니다.
	 * @param Trigger EStateTreeTransitionTrigger 값 이름: OnStateCompleted, OnStateSucceeded, OnStateFailed, OnTick, OnEvent.
	 * @param EventTag Trigger가 OnEvent일 때 기다리는 이벤트.
	 * @param Condition 전환 조건 구조체(예: /Script/Ag.AgEnemyAICondition_PlayerDead). 조건이 없으면 null.
	 * @param ConditionValues 조건 인스턴스 데이터의 값. 언리얼 텍스트 형식이고, 비우면 기본값입니다.
	 * @return 상태나 대상이 없거나, Trigger를 모르거나, 조건을 스키마가 허용하지 않으면 false입니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor", meta = (AICallable))
	static bool AddStateTreeTransition(UStateTree* StateTree, const FString& StatePath, FName Trigger, FGameplayTag EventTag, const FString& TargetPath, EStateTreeTransitionPriority Priority, UScriptStruct* Condition, const FString& ConditionValues);

	/** 상태의 전환을 모두 지웁니다. */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor", meta = (AICallable))
	static bool ClearStateTreeTransitions(UStateTree* StateTree, const FString& StatePath);

	/**
	 * StateTree를 컴파일합니다.
	 * @return 컴파일 메시지(오류와 경고)를 한 줄에 하나씩. 실패하면 첫 줄이 'Failed'이고, 성공하고 메시지가 없으면 빈 문자열입니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor", meta = (AICallable))
	static FString CompileStateTree(UStateTree* StateTree);
};
