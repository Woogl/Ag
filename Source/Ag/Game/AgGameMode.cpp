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
#include "UI/AgBattleResultScreen.h"
#include "UI/AgHUDWidget.h"
#include "UI/AgTitleScreen.h"
#include "UI/AgUILayout.h"

AAgGameMode::AAgGameMode()
{
	PlayerControllerClass = AAgPlayerController::StaticClass();
	GameStateClass = AAgGameState::StaticClass();
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

	// 엔진은 플레이어 폰을 월드 BeginPlay 전에 스폰하므로, 이 순회가 플레이어도 등록합니다.
	AAgGameState* StageState = GetGameState<AAgGameState>();
	for (TActorIterator<AAgCharacterBase> It(GetWorld()); It; ++It)
	{
		It->OnDied.AddUObject(this, &ThisClass::HandleCombatantDied);
		if (AAgBossCharacter* BossCharacter = Cast<AAgBossCharacter>(*It); BossCharacter && StageState)
		{
			StageState->SetBoss(BossCharacter);
		}
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
		// HUD는 만들어질 때 폰의 ASC를 한 번 구독하므로, 폰을 스폰하고 빙의시키는 Super 뒤에 엽니다.
		if (HUDWidgetClass)
		{
			PlayerController->PushScreen(EAgUILayer::HUD, HUDWidgetClass);
		}
	}
}

void AAgGameMode::HandleCombatantDied(AAgCharacterBase* DeadCharacter)
{
	const bool bBossDied = DeadCharacter->IsA<AAgBossCharacter>();
	if (!bBossDied && !DeadCharacter->IsA<AAgPlayerCharacter>())
	{
		return;
	}

	AAgGameState* StageState = GetGameState<AAgGameState>();
	if (!StageState || !StageState->DecideStageResult(bBossDied ? EAgStageResult::Clear : EAgStageResult::GameOver))
	{
		return;
	}

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
		PlayerController->PushScreen(EAgUILayer::Menu, BattleResultScreenClass);
	}
}
