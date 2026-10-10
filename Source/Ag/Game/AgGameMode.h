// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AgGameMode.generated.h"

/**
 * Base game mode. Holds the level transitions shared by the title screen and the result UI (게임 플로우).
 * Level assets come from UAgSettings.
 */
UCLASS(Abstract)
class AAgGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AAgGameMode();

	/** Opens LV_BossStage from the start, resetting every state. */
	void OpenBossStage();

	/** Returns to LV_FrontEnd. */
	void OpenFrontEnd();

	/** Ends the game. */
	void QuitGame();
};
