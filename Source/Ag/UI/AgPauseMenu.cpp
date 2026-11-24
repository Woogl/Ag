// Copyright Woogle. All Rights Reserved.

#include "UI/AgPauseMenu.h"

#include "CommonActivatableWidgetSwitcher.h"
#include "GameFramework/PlayerController.h"
#include "UI/AgTabList.h"

UAgPauseMenu::UAgPauseMenu(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
}

void UAgPauseMenu::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	TabList->SetLinkedSwitcher(TabContent);
}

void UAgPauseMenu::NativeOnActivated()
{
	Super::NativeOnActivated();

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		PlayerController->SetPause(true);
	}
}

void UAgPauseMenu::NativeOnDeactivated()
{
	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		PlayerController->SetPause(false);
	}

	Super::NativeOnDeactivated();
}
