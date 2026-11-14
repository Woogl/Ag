// Copyright Woogle. All Rights Reserved.

#include "Game/AgGameState.h"

void AAgGameState::DecideStageResult(EAgStageResult InStageResult)
{
	if (StageResult == EAgStageResult::None)
	{
		StageResult = InStageResult;
		OnStageResultChanged.Broadcast();
	}
}
