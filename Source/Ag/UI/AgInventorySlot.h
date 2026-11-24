// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "CommonButtonBase.h"
#include "AgInventorySlot.generated.h"

class UCommonLazyImage;
class UCommonTextBlock;

/**
 * Inventory 탭 그리드의 칸입니다. CommonUI 타일 뷰가 버튼 항목의 선택을 리스트의 선택과 맞추므로, 좌클릭으로 고르고
 * 고른 칸과 마우스를 올린 칸의 모양·효과음은 버튼 스타일이 맡습니다.
 */
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgInventorySlot : public UCommonButtonBase, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	UAgInventorySlot(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonLazyImage> Icon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> CountText;
};
