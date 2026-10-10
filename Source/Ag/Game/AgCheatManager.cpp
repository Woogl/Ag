// Copyright Woogle. All Rights Reserved.

#include "Game/AgCheatManager.h"

#include "Character/AgBossCharacter.h"
#include "Character/AgPlayerCharacter.h"
#include "EngineUtils.h"

void UAgCheatManager::AgKillPlayer()
{
	for (TActorIterator<AAgPlayerCharacter> It(GetWorld()); It; ++It)
	{
		It->Die(FVector::ZeroVector);
	}
}

void UAgCheatManager::AgKillBoss()
{
	for (TActorIterator<AAgBossCharacter> It(GetWorld()); It; ++It)
	{
		It->Die(FVector::ZeroVector);
	}
}
