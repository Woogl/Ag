// Copyright Woogle. All Rights Reserved.

#include "Game/AgGameMode.h"

#include "Ag.h"
#include "Character/AgBossCharacter.h"
#include "Character/AgCharacterBase.h"
#include "Character/AgPlayerCharacter.h"
#include "EngineUtils.h"
#include "Game/AgGameState.h"
#include "Game/AgPlayerController.h"
#include "GameMapsSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"
#include "UI/AgHUDWidget.h"
#include "UI/AgTitleScreen.h"
#include "UI/AgUILayout.h"

AAgGameMode::AAgGameMode()
{
	PlayerControllerClass = AAgPlayerController::StaticClass();
	GameStateClass = AAgGameState::StaticClass();
}

void AAgGameMode::RegisterCombatant(AAgCharacterBase* Combatant)
{
	if (Combatant && !RegisteredCombatants.Contains(Combatant))
	{
		RegisteredCombatants.Add(Combatant);
		Combatant->OnDied.AddUObject(this, &ThisClass::HandleCombatantDied);
	}

	AAgBossCharacter* BossCharacter = Cast<AAgBossCharacter>(Combatant);
	AAgGameState* StageState = GetGameState<AAgGameState>();
	if (BossCharacter && StageState)
	{
		StageState->SetBoss(BossCharacter);
	}
}

void AAgGameMode::OpenFrontEnd()
{
	const FSoftObjectPath Map(UGameMapsSettings::GetGameDefaultMap());
	if (Map.IsNull())
	{
		UE_LOG(LogAg, Error, TEXT("Game Default Map is not set (Project Settings > Maps & Modes)."));
		return;
	}
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, TSoftObjectPtr<UWorld>(Map));
}

void AAgGameMode::OpenStage()
{
	if (StageMap.IsNull())
	{
		UE_LOG(LogAg, Error, TEXT("StageMap is not set (BP_FrontEndGameMode)."));
		return;
	}
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, StageMap);
}

void AAgGameMode::RestartStage()
{
	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		PlayerController->RestartLevel();
	}
}

void AAgGameMode::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, UGameplayStatics::GetPlayerController(this, 0), EQuitPreference::Quit, false);
}

void AAgGameMode::BeginPlay()
{
	Super::BeginPlay();

	for (TActorIterator<AAgCharacterBase> It(GetWorld()); It; ++It)
	{
		RegisterCombatant(*It);
	}
}

void AAgGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	if (AAgPlayerController* PlayerController = Cast<AAgPlayerController>(NewPlayer))
	{
		if (TitleScreenClass)
		{
			PlayerController->PushScreen(EAgUILayer::Menu, TitleScreenClass);
		}
		if (HUDWidgetClass)
		{
			PlayerController->ShowHUD(HUDWidgetClass);
		}
	}

	if (AAgCharacterBase* PlayerCharacter = NewPlayer ? Cast<AAgCharacterBase>(NewPlayer->GetPawn()) : nullptr)
	{
		RegisterCombatant(PlayerCharacter);
	}
}

void AAgGameMode::HandleCombatantDied(AAgCharacterBase* DeadCharacter)
{
	const bool bBossDied = DeadCharacter->IsA<AAgBossCharacter>();
	if (!bBossDied && !DeadCharacter->IsA<AAgPlayerCharacter>())
	{
		return;
	}

	if (AAgPlayerController* PlayerController = Cast<AAgPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		PlayerController->HideHUD();
	}

	if (bResultDecided)
	{
		return;
	}
	bResultDecided = true;
	bStageCleared = bBossDied;

	if (StageResultPresentationTime > 0.f)
	{
		GetWorldTimerManager().SetTimer(ResultTimer, this, &ThisClass::ShowResult, StageResultPresentationTime);
	}
	else
	{
		ShowResult();
	}
}

void AAgGameMode::ShowResult()
{
	if (AAgPlayerController* PlayerController = Cast<AAgPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		PlayerController->ShowResult(BattleResultScreenClass, bStageCleared);
	}
}
