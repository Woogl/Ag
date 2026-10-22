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
		RestartButton->OnClicked().AddUObject(GameMode, &AAgBossStageGameMode::RestartStage);
		ReturnToTitleButton->OnClicked().AddUObject(GameMode, &AAgBossStageGameMode::OpenFrontEnd);
		QuitButton->OnClicked().AddUObject(GameMode, &AAgBossStageGameMode::QuitGame);
	}
}

void UAgResultScreen::NativeOnActivated()
{
	Super::NativeOnActivated();

	// ë²„íŠ¼ë§ˆë‹¤ êº¼ì•¼ ?¬ì»¤?¤ë? ë°›ì? ë²„íŠ¼???¤í–‰ ??Enter, Space)ë¥?ë°›ì? ?ŠìŠµ?ˆë‹¤.
	// ë²„íŠ¼???´ì? ?¨ë„ë§??„ë©´ ë§ˆìš°?¤ë§Œ ë§‰íž™?ˆë‹¤.
	for (UAgButton* Button : {RestartButton.Get(), ReturnToTitleButton.Get(), QuitButton.Get()})
	{
		Button->SetIsEnabled(false);
	}
	PlayAnimation(FadeInAnimation);
}

void UAgResultScreen::OnAnimationFinished_Implementation(const UWidgetAnimation* Animation)
{
	Super::OnAnimationFinished_Implementation(Animation);

	if (Animation == FadeInAnimation)
	{
		for (UAgButton* Button : {RestartButton.Get(), ReturnToTitleButton.Get(), QuitButton.Get()})
		{
			Button->SetIsEnabled(true);
		}
		RequestRefreshFocus();
	}
}

UWidget* UAgResultScreen::NativeGetDesiredFocusTarget() const
{
	return RestartButton;
}

TOptional<FUIInputConfig> UAgResultScreen::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}
