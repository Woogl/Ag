// Copyright Woogle. All Rights Reserved.

#include "UI/AgDamageNumberWidget.h"

#include "Animation/WidgetAnimation.h"
#include "CommonTextBlock.h"

void UAgDamageNumberWidget::SetDamage(int32 Amount, bool bLarge, float GameSpeed)
{
	if (bLarge && LargeStyle)
	{
		DamageText->SetStyle(LargeStyle);
	}
	DamageText->SetText(FText::AsNumber(Amount, &FNumberFormattingOptions::DefaultNoGrouping()));
	PlayAnimation(FadeAnimation, 0.f, 1, EUMGSequencePlayMode::Forward, GameSpeed);
}

void UAgDamageNumberWidget::SetGameSpeed(float GameSpeed)
{
	SetPlaybackSpeed(FadeAnimation, GameSpeed);
}

float UAgDamageNumberWidget::GetLifetime() const
{
	return FadeAnimation ? FadeAnimation->GetEndTime() : 0.f;
}
