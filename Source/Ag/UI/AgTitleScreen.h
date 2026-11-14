// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/AgMenuScreen.h"
#include "AgTitleScreen.generated.h"

class UAgButton;
class UAgControlsPopup;
class UAgStageSelectScreen;

UCLASS(Abstract, meta = (DisableNativeTick))
class UAgTitleScreen : public UAgMenuScreen
{
	GENERATED_BODY()

public:
	UAgTitleScreen(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeOnInitialized() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgStageSelectScreen> StageSelectScreenClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgControlsPopup> ControlsPopupClass;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> StartButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> ControlsButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> QuitButton;

private:
	void OpenStageSelect();
	void OpenControls();
};
