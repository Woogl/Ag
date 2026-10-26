// Copyright Woogle. All Rights Reserved.

#include "Game/AgGameState.h"

#include "Character/AgBossCharacter.h"

void AAgGameState::SetBoss(AAgBossCharacter* InBoss)
{
	if (Boss.Get() != InBoss)
	{
		Boss = InBoss;
		OnBossChanged.Broadcast();
	}
}

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
