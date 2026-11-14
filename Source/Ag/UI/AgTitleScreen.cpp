// Copyright Woogle. All Rights Reserved.

#include "UI/AgTitleScreen.h"

#include "Game/AgGameMode.h"
#include "Game/AgPlayerController.h"
#include "UI/AgButton.h"
#include "UI/AgControlsPopup.h"
#include "UI/AgStageSelectScreen.h"
#include "UI/AgUILayout.h"

UAgTitleScreen::UAgTitleScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoRestoreFocus = true;
}

void UAgTitleScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	StartButton->OnClicked().AddUObject(this, &ThisClass::OpenStageSelect);
	ControlsButton->OnClicked().AddUObject(this, &ThisClass::OpenControls);

	if (AAgGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgGameMode>())
	{
		QuitButton->OnClicked().AddUObject(GameMode, &AAgGameMode::QuitGame);
	}
}

UWidget* UAgTitleScreen::NativeGetDesiredFocusTarget() const
{
	return StartButton;
}

void UAgTitleScreen::OpenStageSelect()
{
	if (AAgPlayerController* PlayerController = GetOwningPlayer<AAgPlayerController>())
	{
		PlayerController->PushScreen(EAgUILayer::Menu, StageSelectScreenClass);
	}
}

void UAgTitleScreen::OpenControls()
{
	if (AAgPlayerController* PlayerController = GetOwningPlayer<AAgPlayerController>())
	{
		PlayerController->PushScreen(EAgUILayer::Modal, ControlsPopupClass);
	}
}
