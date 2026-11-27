// Copyright Woogle. All Rights Reserved.

#include "Game/AgGameMode.h"

#include "Ag.h"
#include "Character/AgBossCharacter.h"
#include "Character/AgCharacterBase.h"
#include "Character/AgPlayerCharacter.h"
#include "Camera/AgPlayerCameraManager.h"
#include "Combat/AgTimeSubsystem.h"
#include "CommonActivatableWidget.h"
#include "Components/CapsuleComponent.h"
#include "Engine/AssetManager.h"
#include "EngineUtils.h"
#include "Game/AgEnemySpawner.h"
#include "Game/AgGameState.h"
#include "Game/AgPlayerController.h"
#include "Game/AgSaveSubsystem.h"
#include "GameMapsSettings.h"
#include "Interaction/AgCheckpoint.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"
#include "UI/AgBattleResultScreen.h"
#include "UI/AgHUDWidget.h"
#include "UI/AgTitleScreen.h"
#include "UI/AgUILayout.h"

namespace AgGameModePrivate
{
	const TCHAR* const LevelPrefix = TEXT("LV_");
} // namespace AgGameModePrivate

AAgGameMode::AAgGameMode()
{
	PlayerControllerClass = AAgPlayerController::StaticClass();
	GameStateClass = AAgGameState::StaticClass();
}

TArray<FAgStage> AAgGameMode::FindStages()
{
	// 에셋 레지스트리를 직접 뒤지면 패키지 빌드에서 월드 파티션이 쿡할 때 만든 스트리밍 셀도 레벨로 잡힙니다.
	TArray<FAssetData> Maps;
	UAssetManager::Get().GetPrimaryAssetDataList(UAssetManager::MapType, Maps);
	Maps.Sort([](const FAssetData& A, const FAssetData& B)
	{ return A.AssetName.LexicalLess(B.AssetName); });

	const FName FrontEndMap(FSoftObjectPath(UGameMapsSettings::GetGameDefaultMap()).GetLongPackageName());
	TArray<FAgStage> Stages;
	for (const FAssetData& Map : Maps)
	{
		if (Map.PackageName != FrontEndMap)
		{
			FString Name = Map.AssetName.ToString();
			Name.RemoveFromStart(AgGameModePrivate::LevelPrefix);
			Stages.Add({Map.PackageName, FText::AsCultureInvariant(FName::NameToDisplayString(Name, false))});
		}
	}
	return Stages;
}

void AAgGameMode::OpenFrontEnd()
{
	const FSoftObjectPath Map(UGameMapsSettings::GetGameDefaultMap());
	if (Map.IsNull())
	{
		UE_LOG(LogAg, Error, TEXT("Game Default Map is not set (Project Settings > Maps & Modes)."));
		return;
	}
	UAgSaveSubsystem::Get(this).Unload();
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, TSoftObjectPtr<UWorld>(Map));
}

void AAgGameMode::OpenStage(FName Map)
{
	if (!Map.IsNone())
	{
		UAgSaveSubsystem::Get(this).SetLastStage(Map);
		UGameplayStatics::OpenLevel(this, Map);
	}
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

void AAgGameMode::HandleCharacterDied(AAgCharacterBase& DeadCharacter)
{
	AAgGameState* StageState = GetGameState<AAgGameState>();
	if (StageState->IsStageResultDecided())
	{
		return;
	}

	// 적이 남아 결과가 아직 정해지지 않아도 보스가 죽은 순간에 재생합니다.
	if (const AAgBossCharacter* DeadBoss = Cast<AAgBossCharacter>(&DeadCharacter))
	{
		GetWorld()->GetSubsystem<UAgTimeSubsystem>()->StartSlowMotion(DeadBoss->GetKillSlowMotionSpeed(), DeadBoss->GetKillSlowMotionDuration());
		if (AAgPlayerCameraManager* CameraManager = Cast<AAgPlayerCameraManager>(UGameplayStatics::GetPlayerCameraManager(this, 0)))
		{
			CameraManager->PlayCameraReaction(EAgCameraSituation::BossKill, DeadBoss);
		}
	}

	const bool bPlayerDied = DeadCharacter.IsA<AAgPlayerCharacter>();
	if (!bPlayerDied && !IsStageCleared(DeadCharacter))
	{
		return;
	}
	StageState->DecideStageResult(bPlayerDied ? EAgStageResult::GameOver : EAgStageResult::Clear);
	if (!bPlayerDied)
	{
		// 클리어한 던전은 다음에 들어갈 때 시작 지점에서 처음부터 시작합니다.
		UAgSaveSubsystem::Get(this).ClearDungeonProgress(*GetWorld());
	}
	if (StageResultPresentationTime > 0.f)
	{
		FTimerHandle ResultTimer;
		GetWorldTimerManager().SetTimer(ResultTimer, this, &ThisClass::ShowResult, StageResultPresentationTime);
	}
	else
	{
		ShowResult();
	}
}

void AAgGameMode::RespawnEnemies()
{
	// 전투 서브시스템은 그룹원이 모두 빠져야 그 그룹의 인식 기록을 지우므로, 모두 지운 뒤에 생성합니다.
	for (TActorIterator<AAgEnemySpawner> It(GetWorld()); It; ++It)
	{
		It->DestroyEnemy();
	}
	for (TActorIterator<AAgEnemySpawner> It(GetWorld()); It; ++It)
	{
		It->Spawn();
	}
}

bool AAgGameMode::ShouldRestartOnGameOver() const
{
	return true;
}

void AAgGameMode::StartPlay()
{
	// Super가 모든 액터의 BeginPlay를 부르고, 스포너는 그때 에셋 읽기를 요청합니다.
	Super::StartPlay();

	StartPlayTime = FPlatformTime::Seconds();
	for (TActorIterator<AAgEnemySpawner> It(GetWorld()); It; ++It)
	{
		if (It->IsLoading())
		{
			++PendingSpawnerCount;
			It->HoldSpawn();
			It->OnLoadFinished.AddUObject(this, &ThisClass::HandleSpawnerLoaded);
		}
	}
	SpawnerCount = PendingSpawnerCount;
	AAgPlayerController* PlayerController = Cast<AAgPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	if (PendingSpawnerCount > 0 && PlayerController)
	{
		LoadingScreen = PlayerController->PushScreen(EAgUILayer::Menu, LoadingScreenClass);
	}
}

void AAgGameMode::RestartPlayer(AController* NewPlayer)
{
	const FName CheckpointName = UAgSaveSubsystem::Get(this).GetRestedCheckpoint(*GetWorld());
	const UClass* PawnClass = NewPlayer ? GetDefaultPawnClassForController(NewPlayer) : nullptr;
	const ACharacter* PawnDefaults = PawnClass ? Cast<ACharacter>(PawnClass->GetDefaultObject()) : nullptr;
	if (!CheckpointName.IsNone() && PawnDefaults)
	{
		for (TActorIterator<AAgCheckpoint> It(GetWorld()); It; ++It)
		{
			if (It->GetFName() == CheckpointName)
			{
				// 체크포인트의 시작하는 자리는 발밑이고, 엔진은 받은 자리에 폰의 중심(캡슐 중심)을 둡니다.
				FTransform Start = It->GetStartTransform();
				Start.AddToTranslation(Start.GetUnitAxis(EAxis::Z) * PawnDefaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
				RestartPlayerAtTransform(NewPlayer, Start);
				return;
			}
		}
	}
	Super::RestartPlayer(NewPlayer);
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

void AAgGameMode::HandleSpawnerLoaded()
{
	// 레벨 시작 뒤에 다시 읽기를 기다린 스포너의 알림은 세지 않습니다. 레벨을 닫는 중에 사라지는 스포너의 알림으로는 생성하지 않습니다.
	if (PendingSpawnerCount == 0 || --PendingSpawnerCount > 0 || GetWorld()->bIsTearingDown)
	{
		return;
	}
	// 먼저 읽힌 적이 로딩 화면 뒤에서 움직이지 않게, 모두 읽은 뒤에 한 번에 생성합니다.
	for (TActorIterator<AAgEnemySpawner> It(GetWorld()); It; ++It)
	{
		It->ReleaseSpawn();
	}
	UE_LOG(LogAg, Log, TEXT("Stage ready: %d spawners finished %.3f s after StartPlay."), SpawnerCount, FPlatformTime::Seconds() - StartPlayTime);
	if (LoadingScreen)
	{
		LoadingScreen->DeactivateWidget();
		LoadingScreen = nullptr;
	}
}

void AAgGameMode::ShowResult()
{
	if (AAgPlayerController* PlayerController = Cast<AAgPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		PlayerController->PushScreen(EAgUILayer::Menu, BattleResultScreenClass);
	}
}
