// Copyright Woogle. All Rights Reserved.

#include "UI/AgTitleScreen.h"

#include "Game/AgFrontEndGameMode.h"
#include "Components/VerticalBox.h"
#include "Core/AgAssetManager.h"
#include "Data/AgControlsGuideData.h"
#include "UI/AgButton.h"
#include "UI/AgControlsGuideRow.h"

void UAgTitleScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	ControlsButton->OnClicked().AddUObject(this, &ThisClass::OpenControls);
	CloseButton->OnClicked().AddUObject(this, &ThisClass::CloseControls);
	for (UAgButton* Button : {StartButton.Get(), ControlsButton.Get(), QuitButton.Get(), CloseButton.Get()})
	{
		Button->SetAllNavigationRules(EUINavigationRule::Stop, NAME_None);
	}
	StartButton->SetNavigationRuleExplicit(EUINavigation::Down, ControlsButton);
	ControlsButton->SetNavigationRuleExplicit(EUINavigation::Up, StartButton);
	ControlsButton->SetNavigationRuleExplicit(EUINavigation::Down, QuitButton);
	QuitButton->SetNavigationRuleExplicit(EUINavigation::Up, ControlsButton);

	if (AAgFrontEndGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgFrontEndGameMode>())
	{
		StartButton->OnClicked().AddUObject(GameMode, &AAgFrontEndGameMode::OpenBossStage);
		QuitButton->OnClicked().AddUObject(GameMode, &AAgFrontEndGameMode::QuitGame);
	}
}

void UAgTitleScreen::NativeOnActivated()
{
	bReturnToControls = false;
	SetControlsOpen(false);
	Super::NativeOnActivated();
}

UWidget* UAgTitleScreen::NativeGetDesiredFocusTarget() const
{
	return bControlsOpen ? CloseButton.Get() : (bReturnToControls ? ControlsButton.Get() : StartButton.Get());
}

TOptional<FUIInputConfig> UAgTitleScreen::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void UAgTitleScreen::OpenControls()
{
	if (bControlsOpen || !GuideInputMapping || !ControlsRowClass)
	{
		return;
	}
	ControlsRows->ClearChildren();
	for (const FAgControlsGuideEntry& Entry : UAgAssetManager::Get().GetControlsGuideData().MakeEntries(*GuideInputMapping))
	{
		UAgControlsGuideRow* Row = CreateWidget<UAgControlsGuideRow>(GetOwningPlayer(), ControlsRowClass);
		ControlsRows->AddChildToVerticalBox(Row);
		Row->SetEntry(Entry);
	}
	SetControlsOpen(true);
	CloseButton->SetFocus();
}

void UAgTitleScreen::CloseControls()
{
	if (!bControlsOpen)
	{
		return;
	}
	bReturnToControls = true;
	SetControlsOpen(false);
	ControlsButton->SetFocus();
}

void UAgTitleScreen::SetControlsOpen(bool bOpen)
{
	bControlsOpen = bOpen;
	ControlsPanel->SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	// CommonUI 버튼은 패널만 비활성화하면 키보드 실행이 남습니다.
	for (UAgButton* Button : {StartButton.Get(), ControlsButton.Get(), QuitButton.Get()})
	{
		Button->SetIsEnabled(!bOpen);
	}
	CloseButton->SetIsEnabled(bOpen);
}
