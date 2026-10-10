// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Game/AgGameMode.h"
#include "AgFrontEndGameMode.generated.h"

class UAgTitleScreen;

/** Game mode of LV_FrontEnd (게임 플로우 'Front End'). Spawns no pawn and shows the title screen. */
UCLASS(Abstract)
class AAgFrontEndGameMode : public AAgGameMode
{
	GENERATED_BODY()

public:
	AAgFrontEndGameMode();

protected:
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	/** WBP_Title */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgTitleScreen> TitleScreenClass;
};
