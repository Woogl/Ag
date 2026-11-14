// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/AgMenuScreen.h"
#include "AgControlsPopup.generated.h"

class UAgButton;

UCLASS(Abstract, meta = (DisableNativeTick))
class UAgControlsPopup : public UAgMenuScreen
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> ConfirmButton;
};
