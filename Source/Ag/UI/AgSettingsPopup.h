// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/AgMenuScreen.h"
#include "AgSettingsPopup.generated.h"

class UAgButton;
class UAgDropdown;

UCLASS(Abstract, meta = (DisableNativeTick))
class UAgSettingsPopup : public UAgMenuScreen
{
	GENERATED_BODY()

public:
	UAgSettingsPopup(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgDropdown> LanguageDropdown;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> ConfirmButton;

private:
	void ApplyLanguage();
};
