// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/AgMenuScreen.h"
#include "AgPauseMenu.generated.h"

class UAgTabList;
class UCommonActivatableWidgetSwitcher;

/** 스테이지에서 Esc로 여는 메뉴입니다. 켜져 있는 동안 게임을 멈추고, 탭 바로 내용 화면을 바꿉니다. */
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgPauseMenu : public UAgMenuScreen
{
	GENERATED_BODY()

public:
	UAgPauseMenu(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgTabList> TabList;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetSwitcher> TabContent;
};
