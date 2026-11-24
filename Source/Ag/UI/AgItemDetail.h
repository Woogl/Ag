// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Data/AgItemData.h"
#include "AgItemDetail.generated.h"

class UCommonLazyImage;
class UCommonTextBlock;

/** 아이템 하나의 이름, 카테고리, 보유 수, 아이콘, 설명을 보여 주는 패널입니다. */
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgItemDetail : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void SetItem(FName ItemId, int32 Count);

protected:
	/** 카테고리마다 보여 줄 이름입니다. 문구 표 항목을 가리킵니다. */
	UPROPERTY(EditAnywhere, Category = "Items")
	TMap<EAgItemCategory, FText> CategoryNames;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> NameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> CategoryText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> CountText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonLazyImage> Icon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> DescriptionText;
};
