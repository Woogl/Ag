// Copyright Woogle. All Rights Reserved.

#include "UI/AgSaveSlotRow.h"

#include "CommonTextBlock.h"
#include "Game/AgGameMode.h"
#include "Game/AgSaveSubsystem.h"
#include "UI/AgButton.h"
#include "UI/AgSaveSlotButton.h"

void UAgSaveSlotRow::SetSlot(int32 InSlotIndex, const FAgSaveSlotInfo& Info)
{
	SlotIndex = InSlotIndex;
	const bool bSaved = Info.State == EAgSaveSlotState::Saved;
	LoadButton->SetVisibility(bSaved ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	StatePanel->SetVisibility(bSaved ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	// 접지 않고 숨겨, Delete 버튼이 없는 줄도 다른 줄과 같은 자리에 맞춥니다.
	DeleteButton->SetVisibility(Info.State == EAgSaveSlotState::Empty ? ESlateVisibility::Hidden : ESlateVisibility::Visible);

	if (!bSaved)
	{
		StateSlotNameText->SetText(GetSlotName());
		StateText->SetText(Info.State == EAgSaveSlotState::Empty ? EmptyText : CorruptedText);
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
	LoadButton->SetSlotTexts(GetSlotName(), Stage, PlayTime, FText::AsCultureInvariant(Info.SavedAt.ToString(TEXT("%Y-%m-%d %H:%M"))));
}

FText UAgSaveSlotRow::GetSlotName() const
{
	return FText::Format(SlotNameFormat, FText::AsNumber(SlotIndex + 1));
}

UWidget* UAgSaveSlotRow::GetLoadButton() const
{
	return LoadButton->IsVisible() ? LoadButton.Get() : nullptr;
}

void UAgSaveSlotRow::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	LoadButton->OnClicked().AddWeakLambda(this, [this]()
	{ OnLoadRequested.Broadcast(); });
	DeleteButton->OnClicked().AddWeakLambda(this, [this]()
	{ OnDeleteRequested.Broadcast(); });
}
