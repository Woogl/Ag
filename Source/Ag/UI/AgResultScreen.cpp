// Copyright Woogle. All Rights Reserved.

#include "UI/AgResultScreen.h"

#include "CommonTextBlock.h"
#include "Game/AgGameMode.h"
#include "UI/AgButton.h"

void UAgResultScreen::SetResult(bool bStageCleared)
{
	TitleText->SetText(bStageCleared ? StageClearTitle : GameOverTitle);
}

void UAgResultScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	TryAgainButton->OnClicked().AddUObject(this, &ThisClass::HandleTryAgainClicked);
	ReturnToTitleButton->OnClicked().AddUObject(this, &ThisClass::HandleReturnToTitleClicked);
	QuitButton->OnClicked().AddUObject(this, &ThisClass::HandleQuitClicked);
}

void UAgResultScreen::NativeOnActivated()
{
	Super::NativeOnActivated();

	// Button input starts only after the fade-in (게임 플로우 'Stage Clear UI / Game Over UI').
	ButtonList->SetIsEnabled(false);
	FadeElapsed = 0.f;
	bFadingIn = true;
	SetRenderOpacity(0.f);
	if (FadeInTime <= 0.f)
	{
		FinishFadeIn();
	}
}

void UAgResultScreen::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bFadingIn)
	{
		return;
	}
	FadeElapsed += InDeltaTime;
	SetRenderOpacity(FMath::Clamp(FadeElapsed / FadeInTime, 0.f, 1.f));
	if (FadeElapsed >= FadeInTime)
	{
		FinishFadeIn();
	}
}

void UAgResultScreen::FinishFadeIn()
{
	bFadingIn = false;
	SetRenderOpacity(1.f);
	ButtonList->SetIsEnabled(true);
	RequestRefreshFocus();
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

void UAgResultScreen::HandleTryAgainClicked()
{
	if (AAgGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgGameMode>())
	{
		GameMode->OpenBossStage();
	}
}

void UAgResultScreen::HandleReturnToTitleClicked()
{
	if (AAgGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgGameMode>())
	{
		GameMode->OpenFrontEnd();
	}
}

void UAgResultScreen::HandleQuitClicked()
{
	if (AAgGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgGameMode>())
	{
		GameMode->QuitGame();
	}
}
