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

/** One piece of a montage: a time range of an animation. */
USTRUCT(BlueprintType)
struct FAgMontageSegment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage")
	TObjectPtr<UAnimSequenceBase> Animation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage", meta = (Units = "s"))
	float StartTime = 0.f;

	/** 0 plays to the end of the animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage", meta = (Units = "s"))
	float EndTime = 0.f;

	/** 이 조각의 재생 속도. 1보다 작으면 몽타주에서 더 길게 재생됩니다(A1의 느린 패턴 전조 동작). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage", meta = (ClampMin = 0.01))
	float PlayRate = 1.f;
};

/** One montage section: where it starts and which section plays after it. */
USTRUCT(BlueprintType)
struct FAgMontageSection
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage")
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage", meta = (Units = "s"))
	float StartTime = 0.f;

	/** The section that plays next: none ends the montage, the section's own name repeats it. */
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
	 * Puts a montage slot in its own slot group on the skeleton, so montages in different groups don't stop each other.
	 * @return False if an argument is missing or the slot didn't end up in the group.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor", meta = (AICallable))
	static bool SetSlotGroup(USkeleton* Skeleton, FName SlotName, FName GroupName);

	/**
	 * 몽타주의 트랙을 주어진 슬롯에서 차례로 재생하는 조각들로 바꿉니다. 새 끝보다 뒤에서 시작하는 섹션은 처음으로 옮깁니다.
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
	 * Creates or replaces the widget animation AnimationName: WidgetName's render opacity goes from StartOpacity to
	 * EndOpacity over Duration seconds in a straight line.
	 * @return False if the widget isn't in the widget tree or Duration isn't positive.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor", meta = (AICallable))
	static bool SetFadeAnimation(UWidgetBlueprint* WidgetBlueprint, FName AnimationName, FName WidgetName, float Duration, float StartOpacity, float EndOpacity);

	/** Deletes the widget animation AnimationName the way the widget designer does. False if there was none. */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor", meta = (AICallable))
	static bool RemoveWidgetAnimation(UWidgetBlueprint* WidgetBlueprint, FName AnimationName);

	/**
	 * 스키마 클래스로 새 StateTree 에셋을 만듭니다. 'Root' 상태 하나가 들어 있습니다.
	 * @param AssetPath 예: /Game/AI/ST_Boss
	 * @return 만든 에셋. 그 경로에 에셋이 이미 있거나 만들지 못하면 null입니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor", meta = (AICallable))
	static UStateTree* CreateStateTree(const FString& AssetPath, TSubclassOf<UStateTreeSchema> Schema);

	/**
	 * StateTree 스키마의 속성 하나를 텍스트 값으로 바꾸고, 에디터에서 고친 것처럼 변경을 알립니다.
	 * 예: AI 스키마의 AIControllerClass, ContextActorClass에 클래스 경로(/Script/Ag.AgBossAIController).
	 * @return 속성이 없거나 값을 읽지 못하면 false입니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor", meta = (AICallable))
	static bool SetStateTreeSchemaProperty(UStateTree* StateTree, FName PropertyName, const FString& Value);

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
