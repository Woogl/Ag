// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "AgTitleScreen.generated.h"

class UAgButton;

/** 타이틀 화면 (게임 플로우 '타이틀 화면'). 커서를 표시하고 입력은 UI만 받습니다. */
UCLASS(Abstract)
class UAgTitleScreen : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> StartButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> QuitButton;
};
