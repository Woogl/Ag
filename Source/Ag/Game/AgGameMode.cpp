// Copyright Woogle. All Rights Reserved.

#include "Game/AgGameMode.h"

#include "Ag.h"
#include "Core/AgSettings.h"
#include "Game/AgPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

AAgGameMode::AAgGameMode()
{
	PlayerControllerClass = AAgPlayerController::StaticClass();
}

void AAgGameMode::OpenBossStage()
{
	const TSoftObjectPtr<UWorld>& Map = UAgSettings::Get()->BossStageMap;
	if (Map.IsNull())
	{
		UE_LOG(LogAg, Error, TEXT("UAgSettings::BossStageMap is not set."));
		return;
	}
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, Map);
}

void AAgGameMode::OpenFrontEnd()
{
	const TSoftObjectPtr<UWorld>& Map = UAgSettings::Get()->FrontEndMap;
	if (Map.IsNull())
	{
		UE_LOG(LogAg, Error, TEXT("UAgSettings::FrontEndMap is not set."));
		return;
	}
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, Map);
}

void AAgGameMode::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, UGameplayStatics::GetPlayerController(this, 0), EQuitPreference::Quit, false);
}
