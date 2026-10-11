// Copyright Woogle. All Rights Reserved.

#include "UI/AgDamageFloaterWidget.h"

#include "Animation/WidgetAnimation.h"
#include "CommonTextBlock.h"

void UAgDamageFloaterWidget::SetDamage(int32 Amount)
{
	DamageText->SetText(FText::AsNumber(Amount, &FNumberFormattingOptions::DefaultNoGrouping()));
	PlayAnimation(FadeAnimation);
}

float UAgDamageFloaterWidget::GetLifetime() const
{
	return FadeAnimation ? FadeAnimation->GetEndTime() : 0.f;
}
