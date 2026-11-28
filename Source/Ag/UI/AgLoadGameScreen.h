// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/AgSaveSlotScreen.h"
#include "AgLoadGameScreen.generated.h"

/** 타이틀의 Load Game 화면입니다. 저장된 슬롯의 줄을 실행하면 불러오고, Delete로 지웁니다. */
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgLoadGameScreen : public UAgSaveSlotScreen
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;

	virtual void HandleSlotPicked(int32 SlotIndex) override;

	/** 저장한 시각이 가장 늦은 저장된 슬롯입니다. */
	virtual int32 FindDefaultFocusSlot(const TArray<FAgSaveSlotInfo>& Slots) const override;

	/** 삭제 확인 창의 문구입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	FText DeleteMessage;

private:
	void ConfirmDelete(int32 SlotIndex);
	void DeleteSlot(int32 SlotIndex);
};
