// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AgPlayerController.generated.h"

class UCommonActivatableWidget;
class UAgHUDWidget;
class UAgResultScreen;

/** Player controller. Owns the screens (title, HUD, result UI) and the test console commands. */
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

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UAgHUDWidget> HUDWidget;
};
