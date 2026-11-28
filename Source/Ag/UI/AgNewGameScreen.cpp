// Copyright Woogle. All Rights Reserved.

#include "UI/AgNewGameScreen.h"

#include "Game/AgSaveSubsystem.h"

UAgNewGameScreen::UAgNewGameScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bPickAnySlot = true;
}

void UAgNewGameScreen::HandleSlotPicked(int32 SlotIndex)
{
	if (UAgSaveSubsystem::Get(this).ReadSlots()[SlotIndex].State == EAgSaveSlotState::Empty)
	{
		StartNewGame(SlotIndex);
	}
	else
	{
		OpenConfirmDialog(SlotIndex, OverwriteMessage, FSimpleDelegate::CreateUObject(this, &ThisClass::StartNewGame, SlotIndex));
	}
}

int32 UAgNewGameScreen::FindDefaultFocusSlot(const TArray<FAgSaveSlotInfo>& Slots) const
{
	const int32 Empty = Slots.IndexOfByPredicate([](const FAgSaveSlotInfo& Info)
	{ return Info.State == EAgSaveSlotState::Empty; });
	return Empty != INDEX_NONE ? Empty : 0;
}

void UAgNewGameScreen::StartNewGame(int32 SlotIndex)
{
	UAgSaveSubsystem::Get(this).StartNewGame(SlotIndex);
	OpenStageSelect(SlotIndex);
}
