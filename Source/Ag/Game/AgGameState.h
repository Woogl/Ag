// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "AgGameState.generated.h"

enum class EAgStageResult : uint8
{
	None,
	Clear,
	GameOver
};

UCLASS()
class AAgGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	EAgStageResult GetStageResult() const
	{
		return StageResult;
	}

	/** 결과는 먼저 정해진 것으로 한 번만 정합니다. 이미 정해졌으면 바꾸지 않습니다. */
	void DecideStageResult(EAgStageResult InStageResult);

	FSimpleMulticastDelegate OnStageResultChanged;

private:
	EAgStageResult StageResult = EAgStageResult::None;
};
