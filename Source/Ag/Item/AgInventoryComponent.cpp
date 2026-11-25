// Copyright Woogle. All Rights Reserved.

#include "Item/AgInventoryComponent.h"

#include "Ag.h"
#include "Data/AgItemData.h"

UAgInventoryComponent::UAgInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
}

int32 UAgInventoryComponent::GetCount(FName Item) const
{
	return Counts.FindRef(Item);
}

int32 UAgInventoryComponent::AddItem(FName Item, int32 Count)
{
	const FAgItemData* Data = UAgItemSet::FindItem(Item);
	if (!Data)
	{
		UE_LOG(LogAg, Warning, TEXT("AddItem: item %s is missing in Items"), *Item.ToString());
		return 0;
	}
	int32& Current = Counts.FindOrAdd(Item);
	const int32 Added = FMath::Min(Count, Data->MaxCount - Current);
	if (Added <= 0)
	{
		return 0;
	}
	Current += Added;
	OnItemsChanged.Broadcast();
	return Added;
}

bool UAgInventoryComponent::RemoveItem(FName Item, int32 Count)
{
	int32* Current = Counts.Find(Item);
	if (!Current || Count <= 0 || *Current < Count)
	{
		return false;
	}
	*Current -= Count;
	OnItemsChanged.Broadcast();
	return true;
}

void UAgInventoryComponent::InitializeComponent()
{
	Super::InitializeComponent();

	for (const TPair<FName, FAgItemData>& Pair : UAgItemSet::GetItems())
	{
		Counts.Add(Pair.Key, FMath::Clamp(Pair.Value.StartCount, 0, Pair.Value.MaxCount));
	}
}
