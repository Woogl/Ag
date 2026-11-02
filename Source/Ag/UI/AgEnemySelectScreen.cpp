// Copyright Woogle. All Rights Reserved.

#include "UI/AgEnemySelectScreen.h"

#include "Character/AgCharacterBase.h"
#include "Components/DynamicEntryBox.h"
#include "Core/AgAssetManager.h"
#include "Data/AgCharacterData.h"
#include "Game/AgSimulationRoomGameMode.h"
#include "GameFramework/PlayerController.h"
#include "UI/AgButton.h"
#include "UI/AgDropdown.h"

namespace
{
	/** 엔진 목록은 None을 빈 항목으로 다뤄 고를 수 없으므로, 빈 자리에 따로 키를 둡니다. */
	const FName EmptyOption(TEXT("Empty"));
} // namespace

void UAgEnemySelectScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	AAgSimulationRoomGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgSimulationRoomGameMode>();
	if (!GameMode)
	{
		return;
	}

	for (int32 Index = 0; Index < GameMode->GetSlotCount(); ++Index)
	{
		UAgDropdown* SlotDropdown = SlotList->CreateEntry<UAgDropdown>();
		SlotDropdown->OnSelectionChanged.AddUObject(this, &ThisClass::RefreshSlots);
		SlotDropdowns.Add(SlotDropdown);
	}

	StartButton->OnClicked().AddUObject(this, &ThisClass::StartLineup);
	ReturnToTitleButton->OnClicked().AddUObject(GameMode, &AAgSimulationRoomGameMode::OpenFrontEnd);
	RefreshSlots();
}

void UAgEnemySelectScreen::NativeOnActivated()
{
	Super::NativeOnActivated();

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		PlayerController->SetPause(true);
	}
}

UWidget* UAgEnemySelectScreen::NativeGetDesiredFocusTarget() const
{
	return SlotDropdowns.IsEmpty() ? ReturnToTitleButton.Get() : SlotDropdowns[0]->GetDesiredFocusWidget();
}

TOptional<FUIInputConfig> UAgEnemySelectScreen::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

TArray<TSubclassOf<AAgCharacterBase>> UAgEnemySelectScreen::GetLineup() const
{
	const AAgSimulationRoomGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgSimulationRoomGameMode>();
	TArray<TSubclassOf<AAgCharacterBase>> Lineup;
	for (const UAgDropdown* SlotDropdown : SlotDropdowns)
	{
		const FName Selected = SlotDropdown->GetSelectedOption();
		Lineup.Add(GameMode->FindEnemyClass(Selected == EmptyOption ? NAME_None : Selected));
	}
	return Lineup;
}

void UAgEnemySelectScreen::RefreshSlots()
{
	const AAgSimulationRoomGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgSimulationRoomGameMode>();
	const TArray<TSubclassOf<AAgCharacterBase>> Lineup = GetLineup();
	for (int32 Index = 0; Index < SlotDropdowns.Num(); ++Index)
	{
		TArray<FAgDropdownOption> Options = {{EmptyOption, EmptyText}};
		for (const TSubclassOf<AAgCharacterBase>& EnemyClass : GameMode->GetEnemyClasses())
		{
			if (GameMode->CanPlace(Lineup, Index, EnemyClass))
			{
				const FName Id = EnemyClass->GetDefaultObject<AAgCharacterBase>()->GetCharacterDataId();
				const UAgCharacterData* Data = UAgAssetManager::Get().FindCharacterData(Id);
				Options.Add({Id, Data ? Data->DisplayName : FText::FromName(Id)});
			}
		}
		SlotDropdowns[Index]->SetOptions(Options, SlotDropdowns[Index]->GetSelectedOption());
	}

	// 비활성화하면 포커스를 받을 수 없어, 잠금으로 실행만 막습니다.
	const bool bEmpty = !Lineup.ContainsByPredicate([](const TSubclassOf<AAgCharacterBase>& EnemyClass)
	{ return EnemyClass != nullptr; });
	StartButton->SetIsLocked(bEmpty);
	StartButton->SetRenderOpacity(bEmpty ? LockedButtonOpacity : 1.f);
}

void UAgEnemySelectScreen::StartLineup()
{
	GetWorld()->GetAuthGameMode<AAgSimulationRoomGameMode>()->StartLineup(GetLineup());
}
