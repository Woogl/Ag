// Copyright Woogle. All Rights Reserved.

#include "UI/AgTitleScreen.h"

#include "Game/AgGameMode.h"
#include "Game/AgPlayerController.h"
#include "UI/AgButton.h"
#include "UI/AgSettingsScreen.h"
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

	PlayButton->OnClicked().AddUObject(this, &ThisClass::OpenStageSelect);
	SettingsButton->OnClicked().AddUObject(this, &ThisClass::OpenSettings);

	if (AAgGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgGameMode>())
	{
		QuitButton->OnClicked().AddUObject(GameMode, &AAgGameMode::QuitGame);
	}
}

UWidget* UAgTitleScreen::NativeGetDesiredFocusTarget() const
{
	return PlayButton;
}

void UAgTitleScreen::OpenStageSelect()
{
	if (AAgPlayerController* PlayerController = GetOwningPlayer<AAgPlayerController>())
	{
		PlayerController->PushScreen(EAgUILayer::Menu, StageSelectScreenClass);
	}
}

void UAgTitleScreen::OpenSettings()
{
	if (AAgPlayerController* PlayerController = GetOwningPlayer<AAgPlayerController>())
	{
		PlayerController->PushScreen(EAgUILayer::Menu, SettingsScreenClass);
	}
}
