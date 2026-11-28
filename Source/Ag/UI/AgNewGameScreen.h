// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/AgSaveSlotScreen.h"
#include "AgNewGameScreen.generated.h"

/** 타이틀의 New Game 화면입니다. 고른 슬롯에 새 게임을 만들고, 저장이 있는 슬롯은 덮어쓰기 확인을 거칩니다. */
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgNewGameScreen : public UAgSaveSlotScreen
{
	GENERATED_BODY()

public:
	UAgNewGameScreen(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void HandleSlotPicked(int32 SlotIndex) override;

	/** 빈 슬롯 중 번호가 가장 작은 슬롯이고, 빈 슬롯이 없으면 첫 슬롯입니다. */
	virtual int32 FindDefaultFocusSlot(const TArray<FAgSaveSlotInfo>& Slots) const override;

	/** 덮어쓰기 확인 창의 문구입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	FText OverwriteMessage;

private:
	void StartNewGame(int32 SlotIndex);
};
