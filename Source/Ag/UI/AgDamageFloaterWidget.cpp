// Copyright Woogle. All Rights Reserved.

#include "UI/AgDamageFloaterWidget.h"

#include "Animation/WidgetAnimation.h"
#include "CommonTextBlock.h"

void UAgDamageFloaterWidget::SetDamage(int32 Amount, float GameSpeed)
{
	DamageText->SetText(FText::AsNumber(Amount, &FNumberFormattingOptions::DefaultNoGrouping()));
	PlayAnimation(FadeAnimation, 0.f, 1, EUMGSequencePlayMode::Forward, GameSpeed);
}

void UAgDamageFloaterWidget::SetGameSpeed(float GameSpeed)
{
	SetPlaybackSpeed(FadeAnimation, GameSpeed);
}

float UAgDamageFloaterWidget::GetLifetime() const
{
	return FadeAnimation ? FadeAnimation->GetEndTime() : 0.f;
}
