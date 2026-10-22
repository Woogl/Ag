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

void UAgDamageFloaterWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 위젯 애니메이션은 일시정지와 상관없이 재생되므로, 멈춘 동안은 떠오르기를 맡은 액터와 함께 멈춥니다.
	SetPlaybackSpeed(FadeAnimation, GetWorld()->IsPaused() ? 0.f : 1.f);
}
