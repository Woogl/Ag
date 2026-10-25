// Copyright Woogle. All Rights Reserved.

#include "UI/AgStatBar.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UAgStatBar::SetValue(float Value, float MaxValue, bool bShowDrop)
{
	SetRatio(MaxValue > 0.f ? Value / MaxValue : 0.f, bShowDrop);

	const FIntPoint Values(FMath::RoundToInt(Value), FMath::RoundToInt(MaxValue));
	if (bShowValue && Values != ShownValues)
	{
		ShownValues = Values;
		const FNumberFormattingOptions& Format = FNumberFormattingOptions::DefaultNoGrouping();
		ValueText->SetText(FText::Format(INVTEXT("{0} / {1}"), FText::AsNumber(Values.X, &Format), FText::AsNumber(Values.Y, &Format)));
	}
}

void UAgStatBar::SetRatio(float Ratio, bool bShowDrop)
{
	Bar->SetPercent(Ratio);
	const float PreviousRatio = BarRatio;
	BarRatio = Ratio;
	if (!bShowTrail)
	{
		return;
	}

	// 잔상이 바보다 길면 기다리거나 줄어드는 중이므로, 잔상 없이 줄어들 때는 그 흐름을 그대로 둡니다.
	const bool bDropped = PreviousRatio >= 0.f && Ratio < PreviousRatio;
	if (PreviousRatio < 0.f || TrailRatio <= Ratio || (bDropped && !bShowDrop && TrailRatio <= PreviousRatio))
	{
		GetWorld()->GetTimerManager().ClearTimer(TrailTimer);
		SetTrailRatio(Ratio);
	}
	else if (bDropped && bShowDrop)
	{
		FTimerManager& TimerManager = GetWorld()->GetTimerManager();
		TimerManager.ClearTimer(TrailTimer);
		if (TrailDelay > 0.f)
		{
			TimerManager.SetTimer(TrailTimer, this, &ThisClass::StartTrailShrink, TrailDelay);
		}
		else
		{
			StartTrailShrink();
		}
	}
}

void UAgStatBar::NativePreConstruct()
{
	Super::NativePreConstruct();

	Bar->SetFillColorAndOpacity(FillColor);
	TrailBar->SetVisibility(bShowTrail ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	ValueText->SetVisibility(bShowValue ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UAgStatBar::NativeDestruct()
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TrailTimer);
	}

	Super::NativeDestruct();
}

void UAgStatBar::StartTrailShrink()
{
	TrailShrinkFrom = TrailRatio;
	TrailShrinkStartTime = GetWorld()->GetTimeSeconds();
	UpdateTrailShrink();
}

void UAgStatBar::UpdateTrailShrink()
{
	// 월드 타이머와 월드 시간은 일시정지에 멈추고 슬로우모션에 느려지므로, 잔상도 게임 시간으로 흐릅니다.
	const UWorld* World = GetWorld();
	const float Elapsed = static_cast<float>(World->GetTimeSeconds() - TrailShrinkStartTime);
	const float Alpha = TrailShrinkTime > 0.f ? FMath::Min(Elapsed / TrailShrinkTime, 1.f) : 1.f;
	SetTrailRatio(FMath::Lerp(TrailShrinkFrom, BarRatio, Alpha));
	if (Alpha < 1.f)
	{
		TrailTimer = World->GetTimerManager().SetTimerForNextTick(this, &ThisClass::UpdateTrailShrink);
	}
}

void UAgStatBar::SetTrailRatio(float Ratio)
{
	TrailRatio = Ratio;
	TrailBar->SetPercent(Ratio);
}
