// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "AgHUDWidget.generated.h"

class AAgCharacterBase;
class UProgressBar;

/**
 * Combat HUD (전투 HUD). Parent of WBP_HUD: the layout lives in the widget, this class only feeds it values.
 * While it is active, input goes to the game only and the cursor is hidden (게임 플로우 'Boss Stage').
 */
UCLASS(Abstract)
class UAgHUDWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** 플레이어 상태: HP 바 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> PlayerHPBar;

	/** 보스 상태: HP 바 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> BossHPBar;

private:
	static void SetHPRatio(UProgressBar* Bar, const AAgCharacterBase* Character);

	TWeakObjectPtr<AAgCharacterBase> Boss;
};
