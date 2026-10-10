// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Game/AgGameMode.h"
#include "AgBossStageGameMode.generated.h"

class AAgCharacterBase;
class UAgHUDWidget;
class UAgResultScreen;

/**
 * Game mode of LV_BossStage (게임 플로우 'Boss Stage', 'Stage Clear', 'Game Over').
 * The death processed first decides the result; the result UI follows the dead character's death presentation.
 */
UCLASS(Abstract)
class AAgBossStageGameMode : public AAgGameMode
{
	GENERATED_BODY()

public:
	/** Every player and boss registers itself on BeginPlay. */
	void RegisterCombatant(AAgCharacterBase* Combatant);

protected:
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	/** WBP_HUD */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgHUDWidget> HUDWidgetClass;

	/** WBP_Result */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgResultScreen> ResultScreenClass;

private:
	void HandleCombatantDied(AAgCharacterBase* DeadCharacter);
	void ShowResult();

	bool bResultDecided = false;
	bool bStageCleared = false;
	FTimerHandle ResultTimer;
};
