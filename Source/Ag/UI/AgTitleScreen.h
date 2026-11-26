// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/AgMenuScreen.h"
#include "AgTitleScreen.generated.h"

class UAgButton;
class UAgLoadGameScreen;
class UAgSettingsScreen;
class UAgStageSelectScreen;

UCLASS(Abstract, meta = (DisableNativeTick))
class UAgTitleScreen : public UAgMenuScreen
{
	GENERATED_BODY()

public:
	UAgTitleScreen(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgStageSelectScreen> StageSelectScreenClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgLoadGameScreen> LoadGameScreenClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgSettingsScreen> SettingsScreenClass;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> ContinueButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> NewGameButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> LoadGameButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> SettingsButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> QuitButton;

private:
	void ContinueGame();
	void StartNewGame();
	void OpenScreen(TSubclassOf<UCommonActivatableWidget> ScreenClass);
};
