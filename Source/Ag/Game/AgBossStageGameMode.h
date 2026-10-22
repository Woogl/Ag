// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Game/AgGameMode.h"
#include "AgBossStageGameMode.generated.h"

class AAgCharacterBase;
class UAgHUDWidget;
class UAgResultScreen;

UCLASS(Abstract)
class AAgBossStageGameMode : public AAgGameMode
{
	GENERATED_BODY()

public:
	void RegisterCombatant(AAgCharacterBase* Combatant);

	void RestartStage();

protected:
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgHUDWidget> HUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgResultScreen> ResultScreenClass;

	UPROPERTY(EditDefaultsOnly, Category = "Result")
	float StageResultPresentationTime = 4.f;

private:
	void HandleCombatantDied(AAgCharacterBase* DeadCharacter);
	void ShowResult();

	bool bResultDecided = false;
	bool bStageCleared = false;
	FTimerHandle ResultTimer;
};
