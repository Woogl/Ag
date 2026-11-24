// Copyright Woogle. All Rights Reserved.

#include "UI/AgInventoryTab.h"

#include "Algo/StableSort.h"
#include "CommonTileView.h"
#include "Core/AgAssetManager.h"
#include "Data/AgItemData.h"
#include "Game/AgPlayerController.h"
#include "Item/AgInventoryComponent.h"
#include "UI/AgItemDetail.h"

void UAgInventoryTab::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	ItemGrid->OnItemSelectionChanged().AddUObject(this, &ThisClass::HandleSelectionChanged);
}

void UAgInventoryTab::NativeOnActivated()
{
	Super::NativeOnActivated();

	const AAgPlayerController* PlayerController = GetOwningPlayer<AAgPlayerController>();
	const UAgInventoryComponent* Inventory = PlayerController ? PlayerController->GetInventory() : nullptr;
	const UAgAssetManager& AssetManager = UAgAssetManager::Get();

	TArray<UAgInventoryEntry*> Entries;
	if (Inventory)
	{
		for (const TPair<FName, FAgItemData>& Pair : AssetManager.GetItems())
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
	Algo::StableSortBy(Entries, [&AssetManager](const UAgInventoryEntry* Entry)
	{ return AssetManager.FindItem(Entry->ItemId)->Category; });

	ItemGrid->SetListItems(Entries);
	ItemDetail->SetVisibility(Entries.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	if (!Entries.IsEmpty())
	{
		ItemGrid->SetSelectedIndex(0);
		ItemGrid->ScrollToTop();
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
