// Copyright Woogle. All Rights Reserved.

#include "UI/AgBattleResultScreen.h"

#include "Animation/WidgetAnimation.h"
#include "CommonTextBlock.h"
#include "Game/AgGameMode.h"
#include "UI/AgButton.h"

void UAgBattleResultScreen::SetResult(bool bStageCleared)
{
	TitleText->SetText(bStageCleared ? StageClearTitle : GameOverTitle);
}

void UAgBattleResultScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (AAgGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgGameMode>())
	{
		RestartButton->OnClicked().AddUObject(GameMode, &AAgGameMode::RestartStage);
		ReturnToTitleButton->OnClicked().AddUObject(GameMode, &AAgGameMode::OpenFrontEnd);
		QuitButton->OnClicked().AddUObject(GameMode, &AAgGameMode::QuitGame);
	}
}

void UAgBattleResultScreen::NativeOnActivated()
{
	Super::NativeOnActivated();

	// 버튼마다 꺼야 포커스를 받은 버튼도 실행 키(Enter)를 받지 않습니다.
	// 버튼을 담은 패널만 끄면 마우스만 막힙니다.
	for (UAgButton* Button : {RestartButton.Get(), ReturnToTitleButton.Get(), QuitButton.Get()})
	{
		Button->SetIsEnabled(false);
	}
	PlayAnimation(FadeInAnimation);
}

void UAgBattleResultScreen::OnAnimationFinished_Implementation(const UWidgetAnimation* Animation)
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

UWidget* UAgBattleResultScreen::NativeGetDesiredFocusTarget() const
{
	return RestartButton;
}

TOptional<FUIInputConfig> UAgBattleResultScreen::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}
