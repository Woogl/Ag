// Copyright Woogle. All Rights Reserved.

#include "Game/AgGameState.h"

bool AAgGameState::DecideStageResult(EAgStageResult InStageResult)
{
	if (StageResult != EAgStageResult::None || InStageResult == EAgStageResult::None)
	{
		return false;
	}
	StageResult = InStageResult;
	OnStageResultChanged.Broadcast();
	return true;
}
