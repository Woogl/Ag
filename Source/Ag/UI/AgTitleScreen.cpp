// Copyright Woogle. All Rights Reserved.

#include "UI/AgTitleScreen.h"

#include "Game/AgGameMode.h"
#include "UI/AgButton.h"

void UAgTitleScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	StartButton->OnClicked().AddUObject(this, &ThisClass::HandleStartClicked);
	QuitButton->OnClicked().AddUObject(this, &ThisClass::HandleQuitClicked);
}

UWidget* UAgTitleScreen::NativeGetDesiredFocusTarget() const
{
	// 처음 선택된 버튼은 Start입니다.
	return StartButton;
}

TOptional<FUIInputConfig> UAgTitleScreen::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void UAgTitleScreen::HandleStartClicked()
{
	if (AAgGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgGameMode>())
	{
		GameMode->OpenBossStage();
	}
}

void UAgTitleScreen::HandleQuitClicked()
{
	if (AAgGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgGameMode>())
	{
		GameMode->QuitGame();
	}
}
