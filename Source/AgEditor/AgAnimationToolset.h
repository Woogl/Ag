// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ToolsetRegistry/ToolsetDefinition.h"
#include "AgAnimationToolset.generated.h"

class UAnimMontage;
class UAnimSequenceBase;
class UEdGraph;
class USkeleton;

USTRUCT(BlueprintType)
struct FAgMontageSegment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage")
	TObjectPtr<UAnimSequenceBase> Animation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage")
	float StartTime = 0.f;

	/** 0이면 애니메이션 끝까지 재생합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage")
	float EndTime = 0.f;

	/** 이 조각의 재생 속도. 1보다 작으면 몽타주에서 더 길게 재생됩니다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage")
	float PlayRate = 1.f;
};

USTRUCT(BlueprintType)
struct FAgMontageSection
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage")
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage")
	float StartTime = 0.f;

	/** 다음에 재생할 섹션. None이면 몽타주가 끝나고, 자기 이름이면 반복합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage")
	FName NextSection;
};

/** 고친 에셋은 저장하지 않으므로 에셋 도구로 저장합니다. */
UCLASS(Hidden)
class UAgAnimationToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
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
	 * 애님 블루프린트의 상태 머신 그래프에 상태를 더합니다.
	 * @param bEntry 상태 머신이 시작할 상태이면 true입니다.
	 * @return 상태 안의 그래프입니다. 포즈는 엔진 블루프린트 도구로 넣습니다. 상태 머신 그래프가 아니거나 이름이 겹치면 nullptr입니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor", meta = (AICallable))
	static UEdGraph* AddAnimState(UEdGraph* StateMachineGraph, FName Name, FIntPoint Position, bool bEntry);

	/**
	 * 상태 머신 그래프의 두 상태를 잇는 전환을 더합니다.
	 * @param Priority 같은 상태에서 나가는 전환이 함께 참이면 작은 값이 먼저입니다.
	 * @param bAutomaticRule 재생 중인 시퀀스가 끝나기 크로스페이드 시간 전에 규칙 없이 전환하면 true입니다.
	 * @return 전환 규칙 그래프입니다. 규칙은 엔진 블루프린트 도구로 넣습니다. 상태를 찾지 못하면 nullptr입니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor", meta = (AICallable))
	static UEdGraph* AddAnimStateTransition(UEdGraph* StateMachineGraph, FName From, FName To, int32 Priority, bool bAutomaticRule);
};
