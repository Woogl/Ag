// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AgPlayerController.generated.h"

class UCommonActivatableWidget;
class UAgHUDWidget;
class UAgResultScreen;

/** 플레이어 컨트롤러. 화면(타이틀, HUD, 결과 UI)과 테스트용 콘솔 명령을 맡습니다. */
UCLASS()
class AAgPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AAgPlayerController();

	/** Creates an activatable screen, adds it to the player screen and activates it. */
	UCommonActivatableWidget* PushScreen(TSubclassOf<UCommonActivatableWidget> ScreenClass, int32 ZOrder = 0);

	void ShowHUD(TSubclassOf<UAgHUDWidget> HUDClass);
	void HideHUD();

	/** Shows Stage Clear UI or Game Over UI. */
	void ShowResult(TSubclassOf<UAgResultScreen> ResultScreenClass, bool bStageCleared);

private:
	UPROPERTY()
	TObjectPtr<UAgHUDWidget> HUDWidget;
};
