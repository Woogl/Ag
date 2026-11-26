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
	bHoldsItem = !Entry->IsEmpty();

	// 칸 위젯은 다른 항목에 다시 쓰이는데, 타일 뷰는 고를 수 없는 칸의 버튼만 고를 수 없게 바꾸고 되돌리지는 않습니다.
	SetIsSelectable(bHoldsItem);
	SetIsFocusable(bHoldsItem);
	SetVisibility(bHoldsItem ? ESlateVisibility::Visible : ESlateVisibility::HitTestInvisible);
	Icon->SetVisibility(bHoldsItem ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	CountStrip->SetVisibility(bHoldsItem ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (!bHoldsItem)
	{
		return;
	}

	if (const FAgItemData* Item = UAgItemSet::FindItem(Entry->ItemId))
	{
		Icon->SetBrushFromLazyTexture(Item->Icon);
	}
	CountText->SetText(FText::AsNumber(Entry->Count));
}

bool UAgInventorySlot::IsListItemSelectable() const
{
	return bHoldsItem;
}
