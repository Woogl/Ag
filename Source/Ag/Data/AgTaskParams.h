// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"
#include "AgTaskParams.generated.h"

class FJsonObject;
class UAbilityTask;
class UAgGameplayAbility;
struct FAgJsonFieldRules;

/** Task를 둘 수 있는 곳 */
enum class EAgTaskPlacement : uint8
{
	None = 0,

	/** 행동 정의의 Tasks 배열 */
	Activation = 1 << 0,

	/** 몽타주 시간표의 구간(Start, End) */
	Window = 1 << 1,

	/** 몽타주 시간표의 지점(Time) */
	Point = 1 << 2
};
ENUM_CLASS_FLAGS(EAgTaskPlacement)

/**
 * Task가 Runner에 요청하는 전환. BlendTime이 없으면 같은 재생 안에서 그 섹션으로 옮기고(블렌드 없음), 있으면 그 섹션부터
 * 다시 재생합니다(새 재생, 그 시간의 블렌드 인).
 */
USTRUCT()
struct FAgTransition
{
	GENERATED_BODY()

	UPROPERTY()
	FName Section;

	/** 다시 재생할 때의 블렌드 시간(초). JSON에 적지 않으면 음수이고 같은 재생 안에서 옮깁니다. */
	UPROPERTY()
	float BlendTime = -1.f;

	bool IsSet() const
	{
		return !Section.IsNone();
	}
};

/** Runner가 Task를 만들 때 넘기는 실행 입력 */
struct FAgTaskContext
{
	UAgGameplayAbility& Runner;

	/** 항목을 둔 곳 */
	EAgTaskPlacement Placement = EAgTaskPlacement::Activation;

	/** 시간표 항목이면 그 재생의 몽타주 인스턴스 ID이고, 발동 Task면 행동이 시작한 재생입니다(없으면 INDEX_NONE). */
	int32 MontageInstanceId = INDEX_NONE;
};

/**
 * Task 하나의 Params. Task 이름이 X이면 Params는 이 구조체를 상속한 FAgXParams이고 그 Task의 헤더에 둡니다. 읽을 때 JSON의
 * Task 값으로 구조체를 찾아 FInstancedStruct에 담고, 필드 규칙·검증·Task 생성은 가상 함수가 맡습니다. 구조체 메타데이터는
 * 에디터 전용이라 이름 규칙으로 찾습니다.
 */
USTRUCT()
struct FAgTaskParams
{
	GENERATED_BODY()

	virtual ~FAgTaskParams() = default;

	/** 둘 수 있는 곳 */
	virtual EAgTaskPlacement GetPlacements() const
	{
		return EAgTaskPlacement::None;
	}

	/** 필수가 아니거나 null·음수를 쓸 수 있는 필드. 적지 않은 필드는 모두 필수입니다. */
	virtual void GetFieldRules(FAgJsonFieldRules& Rules) const
	{
	}

	/** 필드를 읽은 뒤 둔 곳(Placement)에 맞는지, 값과 참조를 검사하고, 선택 필드를 적었는지 Json(이 항목의 원문)으로 정합니다. */
	virtual void Validate(const FJsonObject& Json, EAgTaskPlacement Placement, const FString& Path, TArray<FString>& Errors)
	{
	}

	/** 행동 대상을 쓰면 참입니다. 이 항목을 쓰는 행동은 Targeting이나 GameplayEvent 트리거를 가져야 합니다(검증). */
	virtual bool NeedsTarget() const
	{
		return false;
	}

	/** 이 항목이 요청할 수 있는 전환. 없으면 null입니다(검증이 섹션을 확인합니다). */
	virtual const FAgTransition* GetTransition() const
	{
		return nullptr;
	}

	/** 구간에 두면 전환을 기다리는 항목. 기다리는 동안 몽타주가 끝나지 않게 반복 섹션을 끝까지 덮어야 합니다(검증). */
	virtual bool WaitsForTransition() const
	{
		return false;
	}

	/** 이 Params로 Task를 만듭니다. 시작(ReadyForActivation)과 구간의 끝은 Runner가 정합니다. 만들 수 없으면 null입니다. */
	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const
	{
		return nullptr;
	}
};

namespace AgTaskParams
{
	/** Task 이름의 Params 구조체. 등록되지 않은 이름이면 null입니다. */
	const UScriptStruct* FindStruct(const FString& TaskName);

	/** Params 구조체의 Task 이름 */
	FName GetTaskName(const UScriptStruct& Struct);

	/**
	 * {"Task": 이름, 필드...} 항목 하나를 읽어 Out에 담습니다. Placement는 항목을 둔 곳이고, Ignored는 부르는 쪽이 읽는 키(시간표의
	 * Start, End, Time)입니다. 실패하면 Out을 바꾸지 않습니다.
	 */
	bool Read(const FJsonObject& Item, EAgTaskPlacement Placement, FInstancedStruct& Out, const FString& Path, TArray<FString>& Errors, const TArray<FString>& Ignored = TArray<FString>());
}
