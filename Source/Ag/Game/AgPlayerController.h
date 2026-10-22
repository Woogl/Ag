// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AgPlayerController.generated.h"

class UCommonActivatableWidget;
class UAgHUDWidget;
class UAgResultScreen;
class UAgUILayout;
enum class EAgUILayer : uint8;

UCLASS()
class AAgPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AAgPlayerController();

	virtual void ReceivedPlayer() override;

	UCommonActivatableWidget* PushScreen(EAgUILayer Layer, TSubclassOf<UCommonActivatableWidget> ScreenClass);

	void ShowHUD(TSubclassOf<UAgHUDWidget> HUDClass);
	void HideHUD();

	void ShowResult(TSubclassOf<UAgResultScreen> ResultScreenClass, bool bStageCleared);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgUILayout> UILayoutClass;

private:
	UPROPERTY()
	TObjectPtr<UAgUILayout> UILayout;

	UPROPERTY()
	TObjectPtr<UAgHUDWidget> HUDWidget;
};
