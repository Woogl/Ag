// Copyright Woogle. All Rights Reserved.

#include "UI/AgSaveSlotScreen.h"

#include "Components/VerticalBox.h"
#include "Game/AgPlayerController.h"
#include "Game/AgSaveSubsystem.h"
#include "UI/AgButton.h"
#include "UI/AgConfirmDialog.h"
#include "UI/AgSaveSlotRow.h"
#include "UI/AgStageSelectScreen.h"
#include "UI/AgUILayout.h"

UAgSaveSlotScreen::UAgSaveSlotScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
}

void UAgSaveSlotScreen::NativeOnInitialized()
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
		Row->OnSlotPicked.AddUObject(this, &ThisClass::HandleSlotPicked, SlotIndex);
		SlotList->AddChildToVerticalBox(Row);
		Rows.Add(Row);
	}
}

void UAgSaveSlotScreen::NativeOnActivated()
{
	// 엔진은 켜질 때 포커스 대상을 묻으므로 줄을 먼저 채웁니다.
	RefreshSlots();

	Super::NativeOnActivated();
}

UWidget* UAgSaveSlotScreen::NativeGetDesiredFocusTarget() const
{
	UWidget* SlotButton = Rows.IsValidIndex(FocusSlot) ? Rows[FocusSlot]->GetSlotButton() : nullptr;
	return SlotButton ? SlotButton : BackButton.Get();
}

bool UAgSaveSlotScreen::NativeOnHandleBackAction()
{
	// 스택이 닫은 화면을 다시 쓰므로, 다음에 열 때 처음 포커스할 슬롯부터 고르게 비웁니다.
	FocusSlot = INDEX_NONE;
	return Super::NativeOnHandleBackAction();
}

void UAgSaveSlotScreen::RefreshSlots()
{
	const TArray<FAgSaveSlotInfo> Slots = UAgSaveSubsystem::Get(this).ReadSlots();
	for (int32 SlotIndex = 0; SlotIndex < Rows.Num(); ++SlotIndex)
	{
		Rows[SlotIndex]->SetSlot(SlotIndex, Slots[SlotIndex], bPickAnySlot);
	}
	if (!Rows.IsValidIndex(FocusSlot) || !Rows[FocusSlot]->GetSlotButton())
	{
		FocusSlot = FindDefaultFocusSlot(Slots);
	}
}

void UAgSaveSlotScreen::OpenConfirmDialog(int32 SlotIndex, const FText& Message, FSimpleDelegate OnConfirmed)
{
	AAgPlayerController* PlayerController = GetOwningPlayer<AAgPlayerController>();
	UAgConfirmDialog* Dialog = PlayerController ? Cast<UAgConfirmDialog>(PlayerController->PushScreen(EAgUILayer::Modal, ConfirmDialogClass)) : nullptr;
	if (!Dialog)
	{
		return;
	}

	FocusSlot = SlotIndex;
	Dialog->SetMessage(FText::Format(INVTEXT("{0}\n{1}"), Rows[SlotIndex]->GetSlotName(), Message));
	Dialog->OnConfirmed = MoveTemp(OnConfirmed);
	Dialog->OnDeactivated().RemoveAll(this);
	Dialog->OnDeactivated().AddUObject(this, &ThisClass::RequestRefreshFocus);
}

void UAgSaveSlotScreen::OpenStageSelect(int32 SlotIndex)
{
	if (AAgPlayerController* PlayerController = GetOwningPlayer<AAgPlayerController>())
	{
		FocusSlot = SlotIndex;
		PlayerController->PushScreen(EAgUILayer::Menu, StageSelectScreenClass);
	}
}

void UAgSaveSlotScreen::Close()
{
	NativeOnHandleBackAction();
}
