// Copyright Woogle. All Rights Reserved.

#include "Game/AgGameMode.h"

#include "Ag.h"
#include "Character/AgBossCharacter.h"
#include "Character/AgCharacterBase.h"
#include "Character/AgPlayerCharacter.h"
#include "Camera/AgPlayerCameraManager.h"
#include "Combat/AgTimeSubsystem.h"
#include "EngineUtils.h"
#include "Game/AgGameState.h"
#include "Game/AgPlayerController.h"
#include "GameMapsSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/PackageName.h"
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

bool AAgGameMode::IsStageAvailable(int32 StageIndex) const
{
	if (!Stages.IsValidIndex(StageIndex) || Stages[StageIndex].Map.IsNull())
	{
		return false;
	}
	return FPackageName::DoesPackageExist(Stages[StageIndex].Map.GetLongPackageName());
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

void AAgGameMode::OpenStage(int32 StageIndex)
{
	if (!IsStageAvailable(StageIndex))
	{
		UE_LOG(LogAg, Error, TEXT("OpenStage: stage %d has no map in the game (BP_FrontEndGameMode Stages)."), StageIndex);
		return;
	}
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, Stages[StageIndex].Map);
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
	for (TActorIterator<AAgCharacterBase> It(GetWorld()); It; ++It)
	{
		It->OnDied.AddUObject(this, &ThisClass::HandleCombatantDied);
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

bool AAgGameMode::IsStageCleared(const AAgCharacterBase& DeadCharacter) const
{
	return DeadCharacter.IsA<AAgBossCharacter>();
}

void AAgGameMode::HandleCombatantDied(AAgCharacterBase* DeadCharacter)
{
	AAgGameState* StageState = GetGameState<AAgGameState>();
	if (!StageState || StageState->GetStageResult() != EAgStageResult::None)
	{
		return;
	}

	EAgStageResult Result = EAgStageResult::None;
	if (DeadCharacter->IsA<AAgPlayerCharacter>())
	{
		Result = EAgStageResult::GameOver;
	}
	else if (IsStageCleared(*DeadCharacter))
	{
		Result = EAgStageResult::Clear;
	}
	StageState->DecideStageResult(Result);

	// 적이 남아 결과가 아직 정해지지 않아도 보스가 죽은 순간에 재생합니다.
	if (const AAgBossCharacter* DeadBoss = Cast<AAgBossCharacter>(DeadCharacter))
	{
		GetWorld()->GetSubsystem<UAgTimeSubsystem>()->StartSlowMotion(DeadBoss->GetKillSlowMotionSpeed(), DeadBoss->GetKillSlowMotionDuration());
		if (AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
		{
			if (AAgPlayerCameraManager* CameraManager = Player->GetPlayerCameraManager())
			{
				CameraManager->PlayCameraReaction(EAgCameraSituation::BossKill, DeadCharacter);
			}
		}
	}

	if (Result == EAgStageResult::None)
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
