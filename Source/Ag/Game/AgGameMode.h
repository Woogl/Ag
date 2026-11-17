// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AgGameMode.generated.h"

class AAgCharacterBase;
class UAgBattleResultScreen;
class UAgHUDWidget;
class UAgTitleScreen;

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
	void RestartStage();
	void QuitGame();

	/** 죽은 캐릭터가 부릅니다. 레벨에 놓였든 플레이 중에 생겼든 사망은 모두 이리로 옵니다. */
	void HandleCharacterDied(AAgCharacterBase& DeadCharacter);

protected:
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	/** 플레이어가 아닌 캐릭터가 죽었을 때 Stage Clear인지 정합니다. 기본은 죽은 캐릭터가 보스일 때입니다. */
	virtual bool IsStageCleared(const AAgCharacterBase& DeadCharacter) const;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgTitleScreen> TitleScreenClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgHUDWidget> HUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgBattleResultScreen> BattleResultScreenClass;

	UPROPERTY(EditDefaultsOnly, Category = "Result")
	float StageResultPresentationTime = 4.f;

private:
	void ShowResult();
};
