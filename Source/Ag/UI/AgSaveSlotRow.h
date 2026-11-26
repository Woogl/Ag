// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "AgSaveSlotRow.generated.h"

class UAgButton;
class UAgSaveSlotButton;
class UCommonTextBlock;
struct FAgSaveSlotInfo;

/**
 * Load Game 화면의 슬롯 줄 하나입니다. 저장된 슬롯은 불러오기 버튼과 Delete 버튼을, 빈 슬롯과 읽을 수 없는 저장은 버튼 대신
 * 슬롯 이름과 상태 글자만 보여 줍니다. 읽을 수 없는 저장은 Delete 버튼을 남깁니다.
 */
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgSaveSlotRow : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void SetSlot(int32 InSlotIndex, const FAgSaveSlotInfo& Info);

	FText GetSlotName() const;

	/** 불러오기 버튼입니다. 저장된 슬롯이 아니면 nullptr입니다. */
	UWidget* GetLoadButton() const;

	FSimpleMulticastDelegate OnLoadRequested;

	FSimpleMulticastDelegate OnDeleteRequested;

protected:
	virtual void NativeOnInitialized() override;

	/** {0}에 1부터 세는 슬롯 번호가 들어갑니다. */
	UPROPERTY(EditAnywhere, Category = "Slot")
	FText SlotNameFormat;

	UPROPERTY(EditAnywhere, Category = "Slot")
	FText EmptyText;

	UPROPERTY(EditAnywhere, Category = "Slot")
	FText CorruptedText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgSaveSlotButton> LoadButton;

	/** 버튼이 아닌 줄의 표시입니다. 불러오기 버튼과 같은 자리에 놓습니다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> StatePanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> StateSlotNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> StateText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> DeleteButton;

private:
	int32 SlotIndex = INDEX_NONE;
};
