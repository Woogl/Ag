// Copyright Woogle. All Rights Reserved.

#include "UI/AgTitleScreen.h"

#include "Game/AgFrontEndGameMode.h"
#include "UI/AgButton.h"

void UAgTitleScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (AAgFrontEndGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgFrontEndGameMode>())
	{
		StartButton->OnClicked().AddUObject(GameMode, &AAgFrontEndGameMode::OpenBossStage);
		QuitButton->OnClicked().AddUObject(GameMode, &AAgFrontEndGameMode::QuitGame);
	}
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
