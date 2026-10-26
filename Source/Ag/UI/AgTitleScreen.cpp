// Copyright Woogle. All Rights Reserved.

#include "UI/AgTitleScreen.h"

#include "Game/AgGameMode.h"
#include "Game/AgPlayerController.h"
#include "UI/AgButton.h"
#include "UI/AgControlsPopup.h"
#include "UI/AgUILayout.h"

UAgTitleScreen::UAgTitleScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoRestoreFocus = true;
}

void UAgTitleScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	ControlsButton->OnClicked().AddUObject(this, &ThisClass::OpenControls);

	if (AAgGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgGameMode>())
	{
		StartButton->OnClicked().AddUObject(GameMode, &AAgGameMode::OpenStage);
		QuitButton->OnClicked().AddUObject(GameMode, &AAgGameMode::QuitGame);
	}
}

UWidget* UAgTitleScreen::NativeGetDesiredFocusTarget() const
{
	return StartButton;
}

TOptional<FUIInputConfig> UAgTitleScreen::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void UAgTitleScreen::OpenControls()
{
	if (AAgPlayerController* PlayerController = GetOwningPlayer<AAgPlayerController>())
	{
		PlayerController->PushScreen(EAgUILayer::Modal, ControlsPopupClass);
	}
}
