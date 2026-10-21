// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "AgTitleScreen.generated.h"

class UAgButton;

UCLASS(Abstract)
class UAgTitleScreen : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> StartButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> ControlsButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> QuitButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> ControlsPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> CloseButton;

private:
	void OpenControls();
	void CloseControls();
	void SetControlsOpen(bool bOpen);

	bool bControlsOpen = false;
	bool bReturnToControls = false;
};
