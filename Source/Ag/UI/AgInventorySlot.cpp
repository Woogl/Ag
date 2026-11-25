// Copyright Woogle. All Rights Reserved.

#include "UI/AgInventorySlot.h"

#include "CommonLazyImage.h"
#include "CommonTextBlock.h"
#include "Data/AgItemData.h"
#include "UI/AgInventoryTab.h"

UAgInventorySlot::UAgInventorySlot(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bSelectable = true;
}

void UAgInventorySlot::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	const UAgInventoryEntry* Entry = CastChecked<UAgInventoryEntry>(ListItemObject);
	if (const FAgItemData* Item = UAgItemSet::FindItem(Entry->ItemId))
	{
		Icon->SetBrushFromLazyTexture(Item->Icon);
	}
	CountText->SetText(FText::AsNumber(Entry->Count));
}
