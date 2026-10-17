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

/** Runner가 Task를 만들 때 넘기는 실행 입력 */
struct FAgTaskContext
{
	UAgGameplayAbility& Runner;

	/** 시간표 항목이면 그 재생의 몽타주 인스턴스 ID입니다. */
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

	/** 필드를 읽은 뒤 값과 참조를 검사하고, 선택 필드를 적었는지 Json(이 항목의 원문)으로 정합니다. */
	virtual void Validate(const FJsonObject& Json, const FString& Path, TArray<FString>& Errors)
	{
	}

	/** 이 Params로 Task를 만듭니다. 시작(ReadyForActivation)과 끝은 Runner가 정합니다. 행동 처리가 값을 읽는 설정이면 null입니다. */
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
	bool Read(const FJsonObject& Item, EAgTaskPlacement Placement, FInstancedStruct& Out, const FString& Path, TArray<FString>& Errors,
	    const TArray<FString>& Ignored = TArray<FString>());
}
