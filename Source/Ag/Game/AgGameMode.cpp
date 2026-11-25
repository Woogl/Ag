// Copyright Woogle. All Rights Reserved.

#include "Game/AgGameMode.h"

#include "Ag.h"
#include "Character/AgBossCharacter.h"
#include "Character/AgCharacterBase.h"
#include "Character/AgPlayerCharacter.h"
#include "Camera/AgPlayerCameraManager.h"
#include "Combat/AgTimeSubsystem.h"
#include "Engine/AssetManager.h"
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
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, TSoftObjectPtr<UWorld>(Map));
}

void AAgGameMode::OpenStage(FName Map)
{
	if (!Map.IsNone())
	{
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

void AAgGameMode::ShowResult()
{
	if (AAgPlayerController* PlayerController = Cast<AAgPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		PlayerController->PushScreen(EAgUILayer::Menu, BattleResultScreenClass);
	}
}
