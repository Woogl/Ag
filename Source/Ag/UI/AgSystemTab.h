// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "AgSystemTab.generated.h"

class UAgButton;

/** 일시정지 메뉴의 System 탭입니다. 메뉴 닫기(Resume)와 스테이지 이동 버튼을 둡니다. */
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgSystemTab : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> ResumeButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> RestartButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> ReturnToTitleButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> QuitButton;

private:
	void ClosePauseMenu();
};
