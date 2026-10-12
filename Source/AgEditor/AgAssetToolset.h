// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "StateTreeTypes.h"
#include "Templates/SubclassOf.h"
#include "ToolsetRegistry/ToolsetDefinition.h"
#include "AgAssetToolset.generated.h"

class UAnimMontage;
class UBlueprint;
class UAnimSequenceBase;
class USkeleton;
class UStateTree;
class UStateTreeSchema;
class UWidgetBlueprint;

/** 몽타주의 조각 하나: 애니메이션의 한 구간 */
USTRUCT(BlueprintType)
struct FAgMontageSegment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage")
	TObjectPtr<UAnimSequenceBase> Animation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage", meta = (Units = "s"))
	float StartTime = 0.f;

	/** 0이면 애니메이션 끝까지 재생합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage", meta = (Units = "s"))
	float EndTime = 0.f;

	/** 이 조각의 재생 속도. 1보다 작으면 몽타주에서 더 길게 재생됩니다(2연격의 느린 패턴 전조 동작). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage", meta = (ClampMin = 0.01))
	float PlayRate = 1.f;
};

/** 몽타주 섹션 하나: 시작 시각과 다음에 재생할 섹션 */
USTRUCT(BlueprintType)
struct FAgMontageSection
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage")
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage", meta = (Units = "s"))
	float StartTime = 0.f;

	/** 다음에 재생할 섹션. None이면 몽타주가 끝나고, 자기 이름이면 반복합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage")
	FName NextSection;
};

/**
 * 엔진 MCP 도구와 에디터 파이썬으로 할 수 없는 에셋 편집: 몽타주 섹션, 몽타주 구간과 몽타주 길이, 스켈레톤 슬롯 그룹,
 * 위젯 애니메이션, 블루프린트 컴포넌트의 Parent Socket, StateTree 에셋의 생성과 상태·태스크·전환 편집과 컴파일. AgEditor 모듈이 MCP 툴셋으로 등록합니다.
 * 고친 에셋은 저장하지 않으므로 에셋 도구로 저장합니다. StateTree는 경로로 상태를 가리키고(이름을 '/'로 이음, 예:
 * Root/전투), 엔진의 StateTree 조회 도구로 읽습니다.
 */
UCLASS(Hidden)
class UAgAssetToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/**
	 * 블루프린트에서 더한 컴포넌트를 부모 컴포넌트의 소켓에 붙입니다(디테일 패널의 Parent Socket과 같음).
	 * C++에서 만든 상속 컴포넌트는 에디터처럼 바꿀 수 없습니다. 블루프린트를 컴파일합니다.
	 * @return 블루프린트에 그 이름의 컴포넌트가 없으면 false입니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor", meta = (AICallable))
	static bool SetComponentParentSocket(UBlueprint* Blueprint, FName ComponentName, FName SocketName);

	/**
	 * 스켈레톤에서 몽타주 슬롯 SlotName을 슬롯 그룹 GroupName에 넣습니다. 그룹이 다른 몽타주끼리는 서로 멈추지 않습니다.
	 * @return 인자가 빠졌거나 슬롯이 그룹에 들어가지 않았으면 false입니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor", meta = (AICallable))
	static bool SetSlotGroup(USkeleton* Skeleton, FName SlotName, FName GroupName);

	/**
	 * 몽타주의 첫 슬롯 트랙을 SlotName 슬롯에서 차례로 재생하는 조각들로 바꿉니다. 새 끝보다 뒤에서 시작하는 섹션은
	 * 처음으로 옮깁니다.
	 * @param Segments StartTime과 EndTime은 그 조각 애니메이션 안의 시각입니다.
	 * @return 몽타주에 슬롯 트랙이 없거나, Segments가 비었거나, 애니메이션이 없는 조각이 있으면 false이고 몽타주는 그대로입니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor", meta = (AICallable))
	static bool SetMontageSegments(UAnimMontage* Montage, FName SlotName, const TArray<FAgMontageSegment>& Segments);

	/**
	 * 몽타주의 섹션과 그 순서를 바꿉니다(예: 반복하는 가운데 섹션).
	 * @param Sections StartTime은 몽타주 안의 시각입니다.
	 * @return Sections가 비었거나, 이름이 없거나 겹치거나, NextSection이 목록에 없는 이름이면 false이고 몽타주는 그대로입니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor", meta = (AICallable))
	static bool SetMontageSections(UAnimMontage* Montage, const TArray<FAgMontageSection>& Sections);

	/**
	 * 위젯 애니메이션 AnimationName을 만들거나 바꿉니다. WidgetName 위젯의 렌더 불투명도가 Duration초 동안 StartOpacity에서
	 * EndOpacity로 선형으로 바뀝니다.
	 * @return 위젯 트리에 그 위젯이 없거나 Duration이 0 이하면 false입니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor", meta = (AICallable))
	static bool SetFadeAnimation(UWidgetBlueprint* WidgetBlueprint, FName AnimationName, FName WidgetName, float Duration, float StartOpacity, float EndOpacity);

	/** 위젯 애니메이션 AnimationName을 위젯 디자이너와 같은 방식으로 지웁니다. 없었으면 false입니다. */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor", meta = (AICallable))
	static bool RemoveWidgetAnimation(UWidgetBlueprint* WidgetBlueprint, FName AnimationName);

	/**
	 * 스키마 클래스로 새 StateTree 에셋을 만듭니다. 'Root' 상태 하나가 들어 있습니다.
	 * 스키마의 속성(AI 스키마의 AIControllerClass, ContextActorClass 등)은 엔진의 ObjectTools.set_properties로
	 * 에디터 데이터의 Schema 하위 객체에 씁니다.
	 * @param AssetPath 예: /Game/AI/ST_Boss
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
	 * @param Task 태스크 구조체(예: /Script/Ag.AgBossAITask_Wait). 스키마가 허용하는 태스크여야 합니다.
	 * @param InstanceValues 태스크 인스턴스 데이터의 값. 언리얼 텍스트 형식이고(예: (Wait=AfterPattern)), 비우면 기본값입니다.
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
	 * @param Condition 전환 조건 구조체(예: /Script/Ag.AgBossAICondition_PlayerDead). 조건이 없으면 null.
	 * @param ConditionValues 조건 인스턴스 데이터의 값. 언리얼 텍스트 형식이고, 비우면 기본값입니다.
	 * @return 상태나 대상이 없거나, Trigger를 모르거나, 조건을 스키마가 허용하지 않으면 false입니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor", meta = (AICallable))
	static bool AddStateTreeTransition(UStateTree* StateTree, const FString& StatePath, FName Trigger, FGameplayTag EventTag, const FString& TargetPath,
		EStateTreeTransitionPriority Priority, UScriptStruct* Condition, const FString& ConditionValues);

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
