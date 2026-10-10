// Copyright Woogle. All Rights Reserved.

#include "UI/AgDamageNumberWidget.h"

#include "CommonTextBlock.h"

void UAgDamageNumberWidget::SetDamage(int32 Amount, bool bLarge)
{
	if (bLarge && LargeStyle)
	{
		DamageText->SetStyle(LargeStyle);
	}
	DamageText->SetText(FText::AsNumber(Amount, &FNumberFormattingOptions::DefaultNoGrouping()));
}
