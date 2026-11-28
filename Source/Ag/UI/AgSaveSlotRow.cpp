// Copyright Woogle. All Rights Reserved.

#include "UI/AgSaveSlotRow.h"

#include "CommonTextBlock.h"
#include "Game/AgGameMode.h"
#include "Game/AgSaveSubsystem.h"
#include "UI/AgButton.h"
#include "UI/AgSaveSlotButton.h"

void UAgSaveSlotRow::SetSlot(int32 InSlotIndex, const FAgSaveSlotInfo& Info, bool bPickAnySlot)
{
	SlotIndex = InSlotIndex;
	const bool bSaved = Info.State == EAgSaveSlotState::Saved;
	const bool bButton = bSaved || bPickAnySlot;
	SlotButton->SetVisibility(bButton ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	StatePanel->SetVisibility(bButton ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	if (bPickAnySlot)
	{
		DeleteButton->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		// 접지 않고 숨겨, Delete 버튼이 없는 줄도 다른 줄과 같은 자리에 맞춥니다.
		DeleteButton->SetVisibility(Info.State == EAgSaveSlotState::Empty ? ESlateVisibility::Hidden : ESlateVisibility::Visible);
	}

	const FText StatusText = Info.State == EAgSaveSlotState::Empty ? EmptyText : CorruptedText;
	if (!bSaved)
	{
		StateSlotNameText->SetText(GetSlotName());
		StateText->SetText(StatusText);
		SlotButton->SetSlotTexts(GetSlotName(), StatusText, FText::GetEmpty(), FText::GetEmpty());
		return;
	}

	// 마지막 스테이지가 없거나 지금 목록에 없으면 비워 둡니다.
	FText Stage;
	for (const FAgStage& Candidate : AAgGameMode::FindStages())
	{
		if (Candidate.Map == Info.LastStage)
		{
			Stage = Candidate.DisplayName;
		}
	}
	const int32 Seconds = FMath::FloorToInt32(Info.PlayTime);
	const FText PlayTime = FText::AsCultureInvariant(FString::Printf(TEXT("%d:%02d:%02d"), Seconds / 3600, Seconds / 60 % 60, Seconds % 60));
	SlotButton->SetSlotTexts(GetSlotName(), Stage, PlayTime, FText::AsCultureInvariant(Info.SavedAt.ToString(TEXT("%Y-%m-%d %H:%M"))));
}

FText UAgSaveSlotRow::GetSlotName() const
{
	return FText::Format(SlotNameFormat, FText::AsNumber(SlotIndex + 1));
}

UWidget* UAgSaveSlotRow::GetSlotButton() const
{
	return SlotButton->IsVisible() ? SlotButton.Get() : nullptr;
}

void UAgSaveSlotRow::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SlotButton->OnClicked().AddWeakLambda(this, [this]()
	{ OnSlotPicked.Broadcast(); });
	DeleteButton->OnClicked().AddWeakLambda(this, [this]()
	{ OnDeleteRequested.Broadcast(); });
}
