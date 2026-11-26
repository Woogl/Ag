// Copyright Woogle. All Rights Reserved.

#include "UI/AgInventoryTab.h"

#include "Algo/StableSort.h"
#include "CommonTileView.h"
#include "Data/AgItemData.h"
#include "Game/AgPlayerController.h"
#include "Item/AgInventoryComponent.h"
#include "UI/AgItemDetail.h"

void UAgInventoryTab::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	ItemGrid->OnItemSelectionChanged().AddUObject(this, &ThisClass::HandleSelectionChanged);
	ItemGrid->OnIsItemSelectableOrNavigable().BindLambda([](UObject* Item)
	{ return !CastChecked<UAgInventoryEntry>(Item)->IsEmpty(); });
}

void UAgInventoryTab::NativeOnActivated()
{
	Super::NativeOnActivated();

	const AAgPlayerController* PlayerController = GetOwningPlayer<AAgPlayerController>();
	const UAgInventoryComponent* Inventory = PlayerController ? PlayerController->GetInventory() : nullptr;

	TArray<UAgInventoryEntry*> Entries;
	if (Inventory)
	{
		for (const TPair<FName, FAgItemData>& Pair : UAgItemSet::GetItems())
		{
			const int32 Count = Inventory->GetCount(Pair.Key);
			if (Count > 0)
			{
				UAgInventoryEntry* Entry = NewObject<UAgInventoryEntry>(this);
				Entry->ItemId = Pair.Key;
				Entry->Count = Count;
				Entries.Add(Entry);
			}
		}
	}
	// 같은 카테고리 안에서는 아이템 데이터의 순서를 지킵니다.
	Algo::StableSortBy(Entries, [](const UAgInventoryEntry* Entry)
	{ return UAgItemSet::FindItem(Entry->ItemId)->Category; });

	const int32 ItemCount = Entries.Num();
	const int32 SlotCount = FMath::Max(Columns * VisibleRows, FMath::DivideAndRoundUp(ItemCount, Columns) * Columns);
	while (Entries.Num() < SlotCount)
	{
		Entries.Add(NewObject<UAgInventoryEntry>(this));
	}

	ItemGrid->SetListItems(Entries);
	ItemGrid->ScrollToTop();
	ItemDetail->SetVisibility(ItemCount == 0 ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	if (ItemCount > 0)
	{
		ItemGrid->SetSelectedIndex(0);
	}
}

UWidget* UAgInventoryTab::NativeGetDesiredFocusTarget() const
{
	return ItemGrid;
}

void UAgInventoryTab::HandleSelectionChanged(UObject* Item)
{
	if (const UAgInventoryEntry* Entry = Cast<UAgInventoryEntry>(Item))
	{
		ItemDetail->SetItem(Entry->ItemId, Entry->Count);
	}
}
