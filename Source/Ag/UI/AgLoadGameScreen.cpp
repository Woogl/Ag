// Copyright Woogle. All Rights Reserved.

#include "UI/AgLoadGameScreen.h"

#include "Game/AgGameMode.h"
#include "Game/AgSaveSubsystem.h"
#include "UI/AgSaveSlotRow.h"

void UAgLoadGameScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	for (int32 SlotIndex = 0; SlotIndex < Rows.Num(); ++SlotIndex)
	{
		Rows[SlotIndex]->OnDeleteRequested.AddUObject(this, &ThisClass::ConfirmDelete, SlotIndex);
	}
}

void UAgLoadGameScreen::HandleSlotPicked(int32 SlotIndex)
{
	if (!UAgSaveSubsystem::Get(this).LoadSlot(SlotIndex))
	{
		return;
	}
	AAgGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgGameMode>();
	if (!GameMode || !GameMode->OpenLastStage())
	{
		OpenStageSelect(SlotIndex);
	}
}

int32 UAgLoadGameScreen::FindDefaultFocusSlot(const TArray<FAgSaveSlotInfo>& Slots) const
{
	int32 Latest = INDEX_NONE;
	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		if (Slots[SlotIndex].State == EAgSaveSlotState::Saved && (Latest == INDEX_NONE || Slots[SlotIndex].SavedAt > Slots[Latest].SavedAt))
		{
			Latest = SlotIndex;
		}
	}
	return Latest;
}

void UAgLoadGameScreen::ConfirmDelete(int32 SlotIndex)
{
	OpenConfirmDialog(SlotIndex, DeleteMessage, FSimpleDelegate::CreateUObject(this, &ThisClass::DeleteSlot, SlotIndex));
}

void UAgLoadGameScreen::DeleteSlot(int32 SlotIndex)
{
	UAgSaveSubsystem::Get(this).DeleteSlot(SlotIndex);
	FocusSlot = INDEX_NONE;
	RefreshSlots();
	RequestRefreshFocus();
}
