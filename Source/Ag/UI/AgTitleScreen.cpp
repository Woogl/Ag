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
	for (UAgButton* Button : {StartButton.Get(), ControlsButton.Get(), QuitButton.Get()})
	{
		Button->SetAllNavigationRules(EUINavigationRule::Stop, NAME_None);
	}
	StartButton->SetNavigationRuleExplicit(EUINavigation::Down, ControlsButton);
	ControlsButton->SetNavigationRuleExplicit(EUINavigation::Up, StartButton);
	ControlsButton->SetNavigationRuleExplicit(EUINavigation::Down, QuitButton);
	QuitButton->SetNavigationRuleExplicit(EUINavigation::Up, ControlsButton);

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
