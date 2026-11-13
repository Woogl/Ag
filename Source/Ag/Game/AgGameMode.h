// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AgGameMode.generated.h"

class AAgCharacterBase;
class UAgBattleResultScreen;
class UAgHUDWidget;
class UAgTitleScreen;

USTRUCT()
struct FAgStage
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<UWorld> Map;
};

UCLASS(Abstract)
class AAgGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AAgGameMode();

	const TArray<FAgStage>& GetStages() const
	{
		return Stages;
	}

	/** 그 스테이지의 레벨이 지정되어 있고 게임에 있는지 답합니다. */
	bool IsStageAvailable(int32 StageIndex) const;

	void OpenFrontEnd();
	void OpenStage(int32 StageIndex);
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

	UPROPERTY(EditDefaultsOnly, Category = "Maps")
	TArray<FAgStage> Stages;

	UPROPERTY(EditDefaultsOnly, Category = "Result")
	float StageResultPresentationTime = 4.f;

private:
	void ShowResult();

	FTimerHandle ResultTimer;
};
