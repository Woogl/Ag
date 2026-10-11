// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Game/AgGameMode.h"
#include "AgBossStageGameMode.generated.h"

class AAgCharacterBase;
class UAgHUDWidget;
class UAgResultScreen;

/**
 * LV_BossStage의 게임 모드(게임 플로우 'Boss Stage', 'Stage Clear', 'Game Over').
 * 플레이어와 보스의 사망 가운데 먼저 처리된 것이 결과를 정하고, 결과 UI는 그 캐릭터의 사망 연출 뒤에 띄웁니다.
 */
UCLASS(Abstract)
class AAgBossStageGameMode : public AAgGameMode
{
	GENERATED_BODY()

public:
	/** Every player and boss registers itself on BeginPlay. */
	void RegisterCombatant(AAgCharacterBase* Combatant);

	/** 보스전을 처음부터 다시 시작합니다. 레벨을 다시 열어 모든 상태가 초기화됩니다(결과 UI의 Try Again). */
	void RestartStage();

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
