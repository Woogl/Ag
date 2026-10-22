// Copyright Woogle. All Rights Reserved.

#include "Game/AgFrontEndGameMode.h"

#include "Ag.h"
#include "Game/AgPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UI/AgTitleScreen.h"
#include "UI/AgUILayout.h"

AAgFrontEndGameMode::AAgFrontEndGameMode()
{
	DefaultPawnClass = nullptr;
}

void AAgFrontEndGameMode::OpenBossStage()
{
	if (BossStageMap.IsNull())
	{
		UE_LOG(LogAg, Error, TEXT("BossStageMap is not set (BP_FrontEndGameMode)."));
		return;
	}
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, BossStageMap);
}

void AAgFrontEndGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	if (AAgPlayerController* PlayerController = Cast<AAgPlayerController>(NewPlayer))
	{
		PlayerController->PushScreen(EAgUILayer::Menu, TitleScreenClass);
	}
}
