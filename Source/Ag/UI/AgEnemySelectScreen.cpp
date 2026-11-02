// Copyright Woogle. All Rights Reserved.

#include "UI/AgEnemySelectScreen.h"

#include "Character/AgCharacterBase.h"
#include "Components/DynamicEntryBox.h"
#include "Game/AgSimulationRoomGameMode.h"
#include "GameFramework/PlayerController.h"
#include "UI/AgButton.h"
#include "UI/AgEnemySlotWidget.h"

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
		UAgEnemySlotWidget* EnemySlot = SlotList->CreateEntry<UAgEnemySlotWidget>();
		EnemySlot->OnSelectedEnemyChanged.AddUObject(this, &ThisClass::RefreshSlots);
		EnemySlots.Add(EnemySlot);
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
	return EnemySlots.IsEmpty() ? ReturnToTitleButton.Get() : EnemySlots[0]->GetDesiredFocusWidget();
}

TOptional<FUIInputConfig> UAgEnemySelectScreen::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

TArray<TSubclassOf<AAgCharacterBase>> UAgEnemySelectScreen::GetLineup() const
{
	const AAgSimulationRoomGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgSimulationRoomGameMode>();
	TArray<TSubclassOf<AAgCharacterBase>> Lineup;
	for (const UAgEnemySlotWidget* EnemySlot : EnemySlots)
	{
		Lineup.Add(GameMode->FindEnemyClass(EnemySlot->GetSelectedEnemy()));
	}
	return Lineup;
}

void UAgEnemySelectScreen::RefreshSlots()
{
	const AAgSimulationRoomGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgSimulationRoomGameMode>();
	const TArray<TSubclassOf<AAgCharacterBase>> Lineup = GetLineup();
	for (int32 Index = 0; Index < EnemySlots.Num(); ++Index)
	{
		TArray<FName> Ids;
		for (const TSubclassOf<AAgCharacterBase>& EnemyClass : GameMode->GetEnemyClasses())
		{
			if (GameMode->CanPlace(Lineup, Index, EnemyClass))
			{
				Ids.Add(EnemyClass->GetDefaultObject<AAgCharacterBase>()->GetCharacterDataId());
			}
		}
		EnemySlots[Index]->SetOptions(Ids);
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
