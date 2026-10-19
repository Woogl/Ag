// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AgTaskParams.generated.h"

class UAbilityTask;
class UAgGameplayAbility;

enum class EAgTaskPlacement : uint8
{
	Activation,
	Window,
	Point
};

USTRUCT()
struct FAgTransition
{
	GENERATED_BODY()

	UPROPERTY()
	FName Section;

	UPROPERTY()
	float BlendTime = -1.f;

	bool IsSet() const
	{
		return !Section.IsNone();
	}
};

struct FAgTaskContext
{
	UAgGameplayAbility& Runner;

	EAgTaskPlacement Placement = EAgTaskPlacement::Activation;

	int32 MontageInstanceId = INDEX_NONE;

	float Start = 0.f;
	float End = 0.f;
};

/**
 * 구조체 메타데이터는 에디터 전용이라 Task 이름 X의 Params는 이름 규칙(FAgXParams)으로 찾습니다.
 * 시간표 항목은 시각과 Params를 한 JSON 객체에 적으므로 시각 필드를 공통 부모에 둡니다.
 */
USTRUCT()
struct FAgTaskParams
{
	GENERATED_BODY()

	virtual ~FAgTaskParams() = default;

	UPROPERTY()
	float Time = 0.f;

	UPROPERTY()
	TOptional<float> Start;

	UPROPERTY()
	TOptional<float> End;

	UPROPERTY()
	FName EndSection;

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const
	{
		return nullptr;
	}
};

namespace AgTaskParams
{
	const UScriptStruct* FindStruct(const FString& TaskName);
}
