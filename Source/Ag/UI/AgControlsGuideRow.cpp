// Copyright Woogle. All Rights Reserved.

#include "UI/AgControlsGuideRow.h"

#include "Components/TextBlock.h"
#include "Data/AgControlsGuideData.h"

void UAgControlsGuideRow::SetEntry(const FAgControlsGuideEntry& Entry)
{
	KeysText->SetText(Entry.Keys);
	ActionText->SetText(Entry.Action);
	DescriptionText->SetText(Entry.Description);
	DescriptionText->SetVisibility(Entry.Description.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}
