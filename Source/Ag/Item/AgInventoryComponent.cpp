// Copyright Woogle. All Rights Reserved.

#include "Item/AgInventoryComponent.h"

#include "Ag.h"
#include "Data/AgItemData.h"
#include "Engine/World.h"
#include "Game/AgSaveSubsystem.h"

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
	HandleCountsChanged();
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
	HandleCountsChanged();
	return true;
}

void UAgInventoryComponent::Refill()
{
	for (const TPair<FName, FAgItemData>& Item : UAgItemSet::GetItems())
	{
		if (Item.Value.bRefill)
		{
			AddItem(Item.Key, Item.Value.MaxCount);
		}
	}
}

void UAgInventoryComponent::InitializeComponent()
{
	Super::InitializeComponent();

	// 게임 인스턴스가 없는 월드(블루프린트 미리 보기)에는 저장 서브시스템이 없습니다.
	if (!GetWorld()->IsGameWorld())
	{
		return;
	}

	// 저장에만 있고 지금 아이템 데이터에 없는 아이템은 여기서 빠집니다.
	UAgSaveSubsystem& SaveSubsystem = UAgSaveSubsystem::Get(this);
	for (const TPair<FName, FAgItemData>& Item : UAgItemSet::GetItems())
	{
		Counts.Add(Item.Key, Item.Value.bRefill ? Item.Value.MaxCount : FMath::Clamp(SaveSubsystem.GetItemCounts().FindRef(Item.Key), 0, Item.Value.MaxCount));
	}
	SaveSubsystem.SetItemCounts(Counts);
}

void UAgInventoryComponent::HandleCountsChanged()
{
	UAgSaveSubsystem::Get(this).SetItemCounts(Counts);
	OnItemsChanged.Broadcast();
}
