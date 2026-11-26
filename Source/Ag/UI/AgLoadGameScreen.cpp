// Copyright Woogle. All Rights Reserved.

#include "UI/AgLoadGameScreen.h"

#include "Components/VerticalBox.h"
#include "Game/AgPlayerController.h"
#include "Game/AgSaveSubsystem.h"
#include "UI/AgButton.h"
#include "UI/AgConfirmDialog.h"
#include "UI/AgSaveSlotRow.h"
#include "UI/AgStageSelectScreen.h"
#include "UI/AgUILayout.h"

UAgLoadGameScreen::UAgLoadGameScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
}

void UAgLoadGameScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BackButton->OnClicked().AddUObject(this, &ThisClass::Close);

	if (!SlotRowClass)
	{
		return;
	}
	for (int32 SlotIndex = 0; SlotIndex < UAgSaveSubsystem::SlotCount; ++SlotIndex)
	{
		UAgSaveSlotRow* Row = CreateWidget<UAgSaveSlotRow>(this, SlotRowClass);
		Row->OnLoadRequested.AddUObject(this, &ThisClass::LoadSlot, SlotIndex);
		Row->OnDeleteRequested.AddUObject(this, &ThisClass::ConfirmDelete, SlotIndex);
		SlotList->AddChildToVerticalBox(Row);
		Rows.Add(Row);
	}
}

void UAgLoadGameScreen::NativeOnActivated()
{
	// 엔진은 켜질 때 포커스 대상을 묻으므로 줄을 먼저 채웁니다.
	RefreshSlots();

	Super::NativeOnActivated();
}

UWidget* UAgLoadGameScreen::NativeGetDesiredFocusTarget() const
{
	UWidget* LoadButton = Rows.IsValidIndex(FocusSlot) ? Rows[FocusSlot]->GetLoadButton() : nullptr;
	return LoadButton ? LoadButton : BackButton.Get();
}

bool UAgLoadGameScreen::NativeOnHandleBackAction()
{
	// 스택이 닫은 화면을 다시 쓰므로, 다음에 열 때 저장한 시각이 가장 늦은 슬롯부터 고르게 비웁니다.
	FocusSlot = INDEX_NONE;
	return Super::NativeOnHandleBackAction();
}

void UAgLoadGameScreen::RefreshSlots()
{
	const TArray<FAgSaveSlotInfo> Slots = UAgSaveSubsystem::Get(this).ReadSlots();
	int32 Latest = INDEX_NONE;
	for (int32 SlotIndex = 0; SlotIndex < Rows.Num(); ++SlotIndex)
	{
		Rows[SlotIndex]->SetSlot(SlotIndex, Slots[SlotIndex]);
		if (Slots[SlotIndex].State == EAgSaveSlotState::Saved && (Latest == INDEX_NONE || Slots[SlotIndex].SavedAt > Slots[Latest].SavedAt))
		{
			Latest = SlotIndex;
		}
	}
	if (!Slots.IsValidIndex(FocusSlot) || Slots[FocusSlot].State != EAgSaveSlotState::Saved)
	{
		FocusSlot = Latest;
	}
}

void UAgLoadGameScreen::LoadSlot(int32 SlotIndex)
{
	AAgPlayerController* PlayerController = GetOwningPlayer<AAgPlayerController>();
	if (PlayerController && UAgSaveSubsystem::Get(this).LoadSlot(SlotIndex))
	{
		// 스테이지 선택 화면에서 Back으로 돌아오면 이 줄이 다시 선택됩니다.
		FocusSlot = SlotIndex;
		PlayerController->PushScreen(EAgUILayer::Menu, StageSelectScreenClass);
	}
}

void UAgLoadGameScreen::ConfirmDelete(int32 SlotIndex)
{
	AAgPlayerController* PlayerController = GetOwningPlayer<AAgPlayerController>();
	UAgConfirmDialog* Dialog = PlayerController ? Cast<UAgConfirmDialog>(PlayerController->PushScreen(EAgUILayer::Modal, ConfirmDialogClass)) : nullptr;
	if (!Dialog)
	{
		return;
	}

	// No로 닫으면 지우려던 줄이 다시 선택됩니다.
	FocusSlot = SlotIndex;
	Dialog->SetMessage(FText::Format(INVTEXT("{0}\n{1}"), Rows[SlotIndex]->GetSlotName(), DeleteMessage));
	Dialog->OnConfirmed.BindUObject(this, &ThisClass::DeleteSlot, SlotIndex);
	Dialog->OnDeactivated().RemoveAll(this);
	Dialog->OnDeactivated().AddUObject(this, &ThisClass::RequestRefreshFocus);
}

void UAgLoadGameScreen::DeleteSlot(int32 SlotIndex)
{
	UAgSaveSubsystem::Get(this).DeleteSlot(SlotIndex);
	FocusSlot = INDEX_NONE;
	RefreshSlots();
	RequestRefreshFocus();
}

void UAgLoadGameScreen::Close()
{
	NativeOnHandleBackAction();
}
