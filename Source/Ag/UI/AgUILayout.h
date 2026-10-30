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
	Modal
};

UCLASS(Abstract, meta = (DisableNativeTick))
class UAgUILayout : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void PushScreen(EAgUILayer Layer, TSubclassOf<UCommonActivatableWidget> ScreenClass);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> HUDStack;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> MenuStack;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> ModalStack;
};
