// Copyright Woogle. All Rights Reserved.

#include "UI/AgSystemTab.h"

#include "CommonUILibrary.h"
#include "Game/AgGameMode.h"
#include "UI/AgButton.h"
#include "UI/AgPauseMenu.h"

void UAgSystemTab::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	ResumeButton->OnClicked().AddUObject(this, &ThisClass::ClosePauseMenu);

	if (AAgGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgGameMode>())
	{
		// 접힌 버튼은 상자 크기가 0이 되어, 남은 버튼이 빈자리 없이 놓입니다(버튼 간격은 상자 안쪽 여백).
		RestartButton->SetVisibility(GameMode->ShouldRestartOnGameOver() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
		RestartButton->OnClicked().AddUObject(GameMode, &AAgGameMode::RestartStage);
		ReturnToTitleButton->OnClicked().AddUObject(GameMode, &AAgGameMode::OpenFrontEnd);
		QuitButton->OnClicked().AddUObject(GameMode, &AAgGameMode::QuitGame);
	}
}

UWidget* UAgSystemTab::NativeGetDesiredFocusTarget() const
{
	return ResumeButton;
}

void UAgSystemTab::ClosePauseMenu()
{
	if (UCommonActivatableWidget* PauseMenu = Cast<UCommonActivatableWidget>(UCommonUILibrary::FindParentWidgetOfType(this, UAgPauseMenu::StaticClass())))
	{
		PauseMenu->DeactivateWidget();
	}
}
