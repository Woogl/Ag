// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "AgGameState.generated.h"

class AAgBossCharacter;

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
	AAgBossCharacter* GetBoss() const
	{
		return Boss.Get();
	}

	void SetBoss(AAgBossCharacter* InBoss);

	EAgStageResult GetStageResult() const
	{
		return StageResult;
	}

	/** 결과는 먼저 정해진 것으로 한 번만 정합니다. 이미 정해졌으면 바꾸지 않고 false를 돌려줍니다. */
	bool DecideStageResult(EAgStageResult InStageResult);

	FSimpleMulticastDelegate OnBossChanged;
	FSimpleMulticastDelegate OnStageResultChanged;

private:
	TWeakObjectPtr<AAgBossCharacter> Boss;

	EAgStageResult StageResult = EAgStageResult::None;
};
