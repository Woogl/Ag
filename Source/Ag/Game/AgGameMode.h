// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AgGameMode.generated.h"

class AAgCharacterBase;
class UAgBattleResultScreen;
class UAgHUDWidget;
class UAgTitleScreen;
class UCommonActivatableWidget;

struct FAgStage
{
	FName Map;
	FText DisplayName;
};

UCLASS(Abstract)
class AAgGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AAgGameMode();

	/**
	 * 에셋 매니저의 Map 타입(DefaultGame.ini, /Game/Maps 아래의 레벨)을 이름순으로 돌려줍니다. 같은 설정이 쿡할 레벨도 정합니다.
	 * 게임을 실행하면 처음 열리는 레벨은 뺍니다.
	 */
	static TArray<FAgStage> FindStages();

	void OpenFrontEnd();
	void OpenStage(FName Map);

	/** 불러온 슬롯의 마지막 스테이지가 스테이지 목록에 있으면 그 스테이지로 이동합니다. 이동하지 않으면 false입니다. */
	bool OpenLastStage();
	void RestartStage();
	void QuitGame();

	/** 죽은 캐릭터가 부릅니다. 레벨에 놓였든 플레이 중에 생겼든 사망은 모두 이리로 옵니다. */
	void HandleCharacterDied(AAgCharacterBase& DeadCharacter);

	/** 스포너가 생성한 적을 모두 지우고 다시 생성해, 레벨을 처음 열었을 때의 상태로 되돌립니다. */
	void RespawnEnemies();

	/**
	 * Game Over가 정해지면 버튼 없이 지금 스테이지를 다시 시작하는지 정합니다. 기본은 다시 시작합니다.
	 * 이렇게 다시 시작하는 스테이지는 일시정지 메뉴에도 Restart가 없습니다.
	 */
	virtual bool ShouldRestartOnGameOver() const;

	virtual void StartPlay() override;

	/** 이 던전에서 휴식한 체크포인트가 레벨에 있으면 엔진의 시작 지점 대신 그 체크포인트의 시작하는 자리에 스폰합니다. */
	virtual void RestartPlayer(AController* NewPlayer) override;

protected:
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	/** 플레이어가 아닌 캐릭터가 죽었을 때 Stage Clear인지 정합니다. 기본은 죽은 캐릭터가 보스일 때입니다. */
	virtual bool IsStageCleared(const AAgCharacterBase& DeadCharacter) const;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgTitleScreen> TitleScreenClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgHUDWidget> HUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgBattleResultScreen> BattleResultScreenClass;

	/** 레벨을 연 뒤 스포너가 적을 모두 생성할 때까지 띄우는 화면입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UCommonActivatableWidget> LoadingScreenClass;

	UPROPERTY(EditDefaultsOnly, Category = "Result")
	float StageResultPresentationTime = 4.f;

private:
	void HandleSpawnerLoaded();

	void ShowResult();

	UPROPERTY()
	TObjectPtr<UCommonActivatableWidget> LoadingScreen;

	int32 PendingSpawnerCount = 0;

	int32 SpawnerCount = 0;

	double StartPlayTime = 0.0;
};
