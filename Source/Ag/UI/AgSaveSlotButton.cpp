// Copyright Woogle. All Rights Reserved.

#include "UI/AgSaveSlotButton.h"

#include "CommonTextBlock.h"

void UAgSaveSlotButton::SetSlotTexts(const FText& SlotName, const FText& Stage, const FText& PlayTime, const FText& SavedAt)
{
	LabelText->SetText(SlotName);
	StageText->SetText(Stage);
	PlayTimeText->SetText(PlayTime);
	SavedAtText->SetText(SavedAt);
}
