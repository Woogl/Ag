// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/AgMenuScreen.h"
#include "AgStageSelectScreen.generated.h"

class UAgButton;
class UAgDropdown;

UCLASS(Abstract, meta = (DisableNativeTick))
class UAgStageSelectScreen : public UAgMenuScreen
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgDropdown> StageDropdown;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> StartButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> BackButton;

private:
	void OpenSelectedStage();
};
