// Copyright Woogle. All Rights Reserved.

#include "Game/AgFrontEndGameMode.h"

#include "Game/AgPlayerController.h"
#include "UI/AgTitleScreen.h"

AAgFrontEndGameMode::AAgFrontEndGameMode()
{
	DefaultPawnClass = nullptr;
}

void AAgFrontEndGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	if (AAgPlayerController* PlayerController = Cast<AAgPlayerController>(NewPlayer))
	{
		PlayerController->PushScreen(TitleScreenClass);
	}
}
