// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Game/AgGameMode.h"
#include "AgFrontEndGameMode.generated.h"

class UAgTitleScreen;

/** LV_FrontEnd의 게임 모드(게임 플로우 'Front End'). 폰 없이 타이틀 화면을 띄웁니다. */
UCLASS(Abstract)
class AAgFrontEndGameMode : public AAgGameMode
{
	GENERATED_BODY()

public:
	AAgFrontEndGameMode();

	/** 보스전 레벨을 엽니다(게임 플로우 'Front End'의 Start). */
	void OpenBossStage();

protected:
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	/** WBP_Title */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgTitleScreen> TitleScreenClass;

	/** LV_BossStage */
	UPROPERTY(EditDefaultsOnly, Category = "Maps")
	TSoftObjectPtr<UWorld> BossStageMap;
};
