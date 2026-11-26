// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "AgUILayout.generated.h"

class UCommonActivatableWidget;
class UCommonActivatableWidgetStack;

enum class EAgUILayer : uint8
{
	HUD,
	Menu,
	/** 메뉴 화면을 가리지 않고 그 위에 띄우는 확인 창의 레이어입니다. 스택은 맨 위 화면만 보여 주므로 Menu와 나눕니다. */
	Modal
};

UCLASS(Abstract, meta = (DisableNativeTick))
class UAgUILayout : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UCommonActivatableWidget* PushScreen(EAgUILayer Layer, TSubclassOf<UCommonActivatableWidget> ScreenClass);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> HUDStack;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> MenuStack;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> ModalStack;
};
