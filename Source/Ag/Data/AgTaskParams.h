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

	float Start = 0.f;
	float End = 0.f;
	UAgAbilityTask_PlayMontage* Playback = nullptr;
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
