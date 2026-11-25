// Copyright Woogle. All Rights Reserved.

#include "UI/AgItemDetail.h"

#include "CommonLazyImage.h"
#include "CommonTextBlock.h"
#include "Data/AgItemData.h"

void UAgItemDetail::SetItem(FName ItemId, int32 Count)
{
	const FAgItemData* Item = UAgItemSet::FindItem(ItemId);
	if (!Item)
	{
		return;
	}
	NameText->SetText(Item->DisplayName);
	CategoryText->SetText(CategoryNames.FindRef(Item->Category));
	CountText->SetText(FText::Format(INVTEXT("{0} / {1}"), FText::AsNumber(Count), FText::AsNumber(Item->MaxCount)));
	Icon->SetBrushFromLazyTexture(Item->Icon);
	DescriptionText->SetText(Item->Description);
}
