// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AgTaskParams.generated.h"

class UAbilityTask;
class UAgAbilityTask_PlayMontage;
class UAgGameplayAbility;

struct FAgTaskContext
{
	UAgGameplayAbility& Ability;

	/** 타임라인 항목의 구간으로, Params의 섹션 기준 시각과 달리 몽타주 기준 시각입니다. 발동 Task에는 없습니다. */
	TOptional<FFloatInterval> Window;
	UAgAbilityTask_PlayMontage* Playback = nullptr;
};

/**
 * 구조체 메타데이터는 에디터 전용이라 Task 이름 X의 Params는 이름 규칙(FAgXParams)으로 찾습니다.
 * 타임라인 항목은 시각과 Params를 한 JSON 객체에 적으므로 시각 필드를 공통 부모에 둡니다.
 */
USTRUCT()
struct FAgTaskParams
{
	GENERATED_BODY()

	virtual ~FAgTaskParams() = default;

	UPROPERTY()
	float Start = 0.f;

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
