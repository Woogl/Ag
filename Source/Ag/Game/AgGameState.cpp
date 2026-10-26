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

void AAgGameState::SetStageResult(EAgStageResult InStageResult)
{
	if (StageResult != InStageResult)
	{
		StageResult = InStageResult;
		OnStageResultChanged.Broadcast();
	}
}
