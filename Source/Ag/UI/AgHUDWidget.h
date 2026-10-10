// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "AgHUDWidget.generated.h"

/**
 * Combat HUD (전투 HUD). Parent of WBP_HUD.
 * While it is active, input goes to the game only and the cursor is hidden (게임 플로우 'Boss Stage').
 */
UCLASS(Abstract)
class UAgHUDWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
};
