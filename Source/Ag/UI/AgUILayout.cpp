// Copyright Woogle. All Rights Reserved.

#include "UI/AgUILayout.h"

#include "CommonActivatableWidget.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

void UAgUILayout::PushScreen(EAgUILayer Layer, TSubclassOf<UCommonActivatableWidget> ScreenClass)
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
	}
	Stack->AddWidget(ScreenClass);
}
