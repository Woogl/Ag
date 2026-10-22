// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "AgTitleScreen.generated.h"

class UAgButton;
class UAgControlsPopup;

UCLASS(Abstract)
class UAgTitleScreen : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UAgTitleScreen(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeOnInitialized() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgControlsPopup> ControlsPopupClass;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> StartButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> ControlsButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> QuitButton;

private:
	void OpenControls();
};
