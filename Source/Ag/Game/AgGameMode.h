// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AgGameMode.generated.h"

class AAgCharacterBase;
class UAgBattleResultScreen;
class UAgHUDWidget;
class UAgTitleScreen;

UCLASS(Abstract)
class AAgGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AAgGameMode();

	void OpenFrontEnd();
	void OpenStage();
	void RestartStage();
	void QuitGame();

protected:
	virtual void BeginPlay() override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	/** 플레이어가 아닌 전투원이 죽었을 때 Stage Clear인지 정합니다. 기본은 죽은 캐릭터가 보스일 때입니다. */
	virtual bool IsStageCleared(const AAgCharacterBase& DeadCharacter) const;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgTitleScreen> TitleScreenClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgHUDWidget> HUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgBattleResultScreen> BattleResultScreenClass;

	UPROPERTY(EditDefaultsOnly, Category = "Maps")
	TSoftObjectPtr<UWorld> StageMap;

	UPROPERTY(EditDefaultsOnly, Category = "Result")
	float StageResultPresentationTime = 4.f;

private:
	void HandleCombatantDied(AAgCharacterBase* DeadCharacter);
	void ShowResult();

	FTimerHandle ResultTimer;
};
