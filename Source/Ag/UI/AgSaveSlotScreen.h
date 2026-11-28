// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/AgMenuScreen.h"
#include "AgSaveSlotScreen.generated.h"

class UAgButton;
class UAgConfirmDialog;
class UAgSaveSlotRow;
class UAgStageSelectScreen;
class UVerticalBox;
struct FAgSaveSlotInfo;

/**
 * 타이틀에서 슬롯을 고르는 화면(Load Game, New Game)의 부모입니다. 슬롯 줄을 슬롯 수만큼 한 번 만들어 두고, 켜질 때마다
 * 저장 파일을 읽어 다시 채웁니다. 줄을 실행했을 때 할 일과 처음 포커스할 슬롯은 자식이 정합니다.
 */
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgSaveSlotScreen : public UAgMenuScreen
{
	GENERATED_BODY()

public:
	UAgSaveSlotScreen(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual bool NativeOnHandleBackAction() override;

	virtual void HandleSlotPicked(int32 SlotIndex) PURE_VIRTUAL(UAgSaveSlotScreen::HandleSlotPicked, );

	/** 포커스를 줄 슬롯이 정해져 있지 않을 때 고를 슬롯입니다. INDEX_NONE이면 Back 버튼이 포커스를 받습니다. */
	virtual int32 FindDefaultFocusSlot(const TArray<FAgSaveSlotInfo>& Slots) const PURE_VIRTUAL(UAgSaveSlotScreen::FindDefaultFocusSlot, return INDEX_NONE;);

	/** 저장 파일을 읽어 줄을 다시 채웁니다. 포커스를 줄 슬롯의 줄이 버튼이 아니면 FindDefaultFocusSlot으로 바꿉니다. */
	void RefreshSlots();

	/** 확인 창을 Modal 레이어에 엽니다. 창에는 문구 위에 그 슬롯의 이름을 한 줄 더 적고, 창이 닫히면 그 슬롯의 줄이 선택됩니다. */
	void OpenConfirmDialog(int32 SlotIndex, const FText& Message, FSimpleDelegate OnConfirmed);

	/** 스테이지 선택 화면을 엽니다. 그 화면에서 Back으로 돌아오면 그 슬롯의 줄이 선택됩니다. */
	void OpenStageSelect(int32 SlotIndex);

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgSaveSlotRow> SlotRowClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgStageSelectScreen> StageSelectScreenClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgConfirmDialog> ConfirmDialogClass;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> SlotList;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> BackButton;

	UPROPERTY()
	TArray<TObjectPtr<UAgSaveSlotRow>> Rows;

	/** 슬롯 상태와 관계없이 모든 줄을 버튼으로 보입니다. 화면 클래스가 정하는 규칙이라 에셋 기본값으로 두지 않습니다. */
	bool bPickAnySlot = false;

	/** 화면이 포커스를 줄 슬롯입니다. INDEX_NONE이면 Back 버튼이 포커스를 받습니다. */
	int32 FocusSlot = INDEX_NONE;

private:
	void Close();
};
