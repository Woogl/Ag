// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/AgButton.h"
#include "AgSaveSlotButton.generated.h"

class UCommonTextBlock;

/** Load Game 화면에서 저장된 슬롯을 불러오는 버튼입니다. 버튼 글자 자리에 슬롯 이름을 적고, 저장 내용을 함께 보여 줍니다. */
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgSaveSlotButton : public UAgButton
{
	GENERATED_BODY()

public:
	void SetSlotTexts(const FText& SlotName, const FText& Stage, const FText& PlayTime, const FText& SavedAt);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> StageText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> PlayTimeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> SavedAtText;
};
