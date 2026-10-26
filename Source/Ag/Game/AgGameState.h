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

	void SetStageResult(EAgStageResult InStageResult);

	FSimpleMulticastDelegate OnBossChanged;
	FSimpleMulticastDelegate OnStageResultChanged;

private:
	TWeakObjectPtr<AAgBossCharacter> Boss;

	EAgStageResult StageResult = EAgStageResult::None;
};
