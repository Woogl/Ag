// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/AgMenuScreen.h"
#include "AgLoadGameScreen.generated.h"

class UAgButton;
class UAgConfirmDialog;
class UAgSaveSlotRow;
class UAgStageSelectScreen;
class UVerticalBox;

/** 타이틀의 Load Game 화면입니다. 슬롯 줄을 슬롯 수만큼 한 번 만들어 두고, 켜질 때마다 저장 파일을 읽어 다시 채웁니다. */
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgLoadGameScreen : public UAgMenuScreen
{
	GENERATED_BODY()

public:
	UAgLoadGameScreen(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual bool NativeOnHandleBackAction() override;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgSaveSlotRow> SlotRowClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgStageSelectScreen> StageSelectScreenClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgConfirmDialog> ConfirmDialogClass;

	/** 삭제 확인 창의 문구입니다. 창에는 이 문구 위에 지울 슬롯의 이름을 한 줄 더 적습니다. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	FText DeleteMessage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> SlotList;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> BackButton;

private:
	/** 저장 파일을 읽어 줄을 다시 채웁니다. 포커스를 줄 슬롯이 저장된 슬롯이 아니면 저장한 시각이 가장 늦은 슬롯으로 바꿉니다. */
	void RefreshSlots();

	void LoadSlot(int32 SlotIndex);
	void ConfirmDelete(int32 SlotIndex);
	void DeleteSlot(int32 SlotIndex);
	void Close();

	UPROPERTY()
	TArray<TObjectPtr<UAgSaveSlotRow>> Rows;

	/** 화면이 포커스를 줄 슬롯입니다. 저장된 슬롯이 없으면 INDEX_NONE이고 Back 버튼이 포커스를 받습니다. */
	int32 FocusSlot = INDEX_NONE;
};
