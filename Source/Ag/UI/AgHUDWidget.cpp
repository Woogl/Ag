// Copyright Woogle. All Rights Reserved.

#include "UI/AgHUDWidget.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "Character/AgBossCharacter.h"
#include "Components/ProgressBar.h"
#include "EngineUtils.h"

TOptional<FUIInputConfig> UAgHUDWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Game, EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown, /*bHideCursorDuringViewportCapture*/ true);
}

void UAgHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!Boss.IsValid())
	{
		for (TActorIterator<AAgBossCharacter> It(GetWorld()); It; ++It)
		{
			Boss = *It;
			break;
		}
	}

	SetHPRatio(PlayerHPBar, Cast<AAgCharacterBase>(GetOwningPlayerPawn()));
	SetHPRatio(BossHPBar, Boss.Get());
}

void UAgHUDWidget::SetHPRatio(UProgressBar* Bar, const AAgCharacterBase* Character)
{
	if (Bar && Character)
	{
		const UAgAttributeSet* Stats = Character->GetAttributeSet();
		Bar->SetPercent(Stats->GetMaxHP() > 0.f ? Stats->GetHP() / Stats->GetMaxHP() : 0.f);
	}
}
