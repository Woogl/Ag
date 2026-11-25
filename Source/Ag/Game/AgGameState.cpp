// Copyright Woogle. All Rights Reserved.

#include "Game/AgGameState.h"

void AAgGameState::DecideStageResult(EAgStageResult InStageResult)
{
	if (!IsStageResultDecided())
	{
		StageResult = InStageResult;
		OnStageResultChanged.Broadcast();
	}
}
