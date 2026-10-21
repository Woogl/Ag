// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AgPlayerController.generated.h"

class UCommonActivatableWidget;
class UAgHUDWidget;
class UAgResultScreen;

UCLASS()
class AAgPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AAgPlayerController();

	UCommonActivatableWidget* PushScreen(TSubclassOf<UCommonActivatableWidget> ScreenClass, int32 ZOrder = 0);

	void ShowHUD(TSubclassOf<UAgHUDWidget> HUDClass);
	void HideHUD();

	void ShowResult(TSubclassOf<UAgResultScreen> ResultScreenClass, bool bStageCleared);

private:
	UPROPERTY()
	TObjectPtr<UAgHUDWidget> HUDWidget;
};
