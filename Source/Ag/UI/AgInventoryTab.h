// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "AgInventoryTab.generated.h"

class UAgItemDetail;
class UCommonTileView;

/** Inventory 탭 그리드의 항목 하나입니다. 엔진 리스트 뷰는 항목을 UObject로 받습니다. 아이템 ID가 없으면 빈 칸입니다. */
UCLASS()
class UAgInventoryEntry : public UObject
{
	GENERATED_BODY()

public:
	bool IsEmpty() const
	{
		return ItemId.IsNone();
	}

	FName ItemId;

	int32 Count = 0;
};

/**
 * 일시정지 메뉴의 Inventory 탭입니다. 켜질 때마다 가진 아이템으로 그리드를 다시 채우고 남은 자리를 빈 칸으로 채운 뒤
 * 첫 칸을 고르며, 고른 칸의 아이템을 상세 패널에 보입니다. 빈 칸은 고르거나 방향키로 지나갈 수 없습니다. 메뉴가 열린
 * 동안 게임이 멈춰 보유 수가 바뀌지 않으므로 바뀜 알림은 받지 않습니다.
 */
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgInventoryTab : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTileView> ItemGrid;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgItemDetail> ItemDetail;

	/** 그리드 한 줄의 칸 수. 아이템이 이보다 적어도 이 수 × 보이는 줄 수만큼, 넘으면 마지막 줄 끝까지 빈 칸을 둡니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Grid", meta = (ClampMin = 1))
	int32 Columns = 7;

	UPROPERTY(EditDefaultsOnly, Category = "Grid", meta = (ClampMin = 1))
	int32 VisibleRows = 4;

private:
	void HandleSelectionChanged(UObject* Item);
};
