// Copyright Woogle. All Rights Reserved.

#include "Game/AgGameState.h"

void AAgGameState::DecideStageResult(EAgStageResult InStageResult)
{
	if (StageResult != EAgStageResult::None || InStageResult == EAgStageResult::None)
	{
		return;
	}
	StageResult = InStageResult;
	OnStageResultChanged.Broadcast();
}
