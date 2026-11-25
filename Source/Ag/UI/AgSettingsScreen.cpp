// Copyright Woogle. All Rights Reserved.

#include "UI/AgSettingsScreen.h"

#include "UI/AgButton.h"
#include "UI/AgSettingsPanel.h"

UAgSettingsScreen::UAgSettingsScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
}

void UAgSettingsScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BackButton->OnClicked().AddUObject(this, &ThisClass::DeactivateWidget);
}

UWidget* UAgSettingsScreen::NativeGetDesiredFocusTarget() const
{
	return SettingsPanel->GetDesiredFocusTarget();
}
