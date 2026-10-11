// Copyright Woogle. All Rights Reserved.

#include "UI/AgResultScreen.h"

#include "Animation/WidgetAnimation.h"
#include "CommonTextBlock.h"
#include "Game/AgBossStageGameMode.h"
#include "UI/AgButton.h"

void UAgResultScreen::SetResult(bool bStageCleared)
{
	TitleText->SetText(bStageCleared ? StageClearTitle : GameOverTitle);
}

void UAgResultScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (AAgBossStageGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgBossStageGameMode>())
	{
		TryAgainButton->OnClicked().AddUObject(GameMode, &AAgBossStageGameMode::RestartStage);
		ReturnToTitleButton->OnClicked().AddUObject(GameMode, &AAgBossStageGameMode::OpenFrontEnd);
		QuitButton->OnClicked().AddUObject(GameMode, &AAgBossStageGameMode::QuitGame);
	}
}

void UAgResultScreen::NativeOnActivated()
{
	Super::NativeOnActivated();

	// 버튼 입력은 페이드 인이 끝난 뒤부터 받습니다(게임 플로우 'Stage Clear UI / Game Over UI').
	ButtonList->SetIsEnabled(false);
	PlayAnimation(FadeInAnimation);
}

void UAgResultScreen::OnAnimationFinished_Implementation(const UWidgetAnimation* Animation)
{
	Super::OnAnimationFinished_Implementation(Animation);

	if (Animation == FadeInAnimation)
	{
		ButtonList->SetIsEnabled(true);
		RequestRefreshFocus();
	}
}

UWidget* UAgResultScreen::NativeGetDesiredFocusTarget() const
{
	// 처음 선택된 버튼은 Try Again입니다.
	return TryAgainButton;
}

TOptional<FUIInputConfig> UAgResultScreen::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}
