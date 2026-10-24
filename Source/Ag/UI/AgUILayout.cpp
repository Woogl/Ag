// Copyright Woogle. All Rights Reserved.

#include "UI/AgUILayout.h"

#include "CommonActivatableWidget.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

UCommonActivatableWidget* UAgUILayout::PushScreen(EAgUILayer Layer, TSubclassOf<UCommonActivatableWidget> ScreenClass)
{
	UCommonActivatableWidgetStack* Stack = nullptr;
	switch (Layer)
	{
	case EAgUILayer::HUD:
		Stack = HUDStack;
		break;
	case EAgUILayer::Menu:
		Stack = MenuStack;
		break;
	case EAgUILayer::Modal:
		Stack = ModalStack;
		break;
	}
	return Stack->AddWidget(ScreenClass);
}
