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
	Menu
};

UCLASS(Abstract, meta = (DisableNativeTick))
class UAgUILayout : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	/** 스택이 만들거나 다시 쓰는 화면 인스턴스를 돌려줍니다. */
	UCommonActivatableWidget* PushScreen(EAgUILayer Layer, TSubclassOf<UCommonActivatableWidget> ScreenClass);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> HUDStack;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> MenuStack;
};
