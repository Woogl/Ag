// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "AgStageSelectScreen.generated.h"

class UAgButton;
class UAgStageOptionWidget;
class UComboBoxKey;

UCLASS(Abstract, meta = (DisableNativeTick))
class UAgStageSelectScreen : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgStageOptionWidget> OptionWidgetClass;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxKey> StageComboBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> StartButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> BackButton;

private:
	UFUNCTION()
	UWidget* GenerateOptionWidget(FName Option);

	void OpenSelectedStage();
};
