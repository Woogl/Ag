// Copyright Woogle. All Rights Reserved.

#include "Game/AgSaveSubsystem.h"

#include "Ag.h"
#include "Data/AgItemData.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Game/AgSaveGame.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/DateTime.h"

namespace AgSaveSubsystemPrivate
{
	// 엔진이 저장 파일 맨 앞에 쓰는 표식("SAVG")입니다. 엔진은 표식이 다르면 옛 형식으로 보고 파일의 처음을 클래스 이름의
	// 길이로 읽어 그만큼 메모리를 잡으므로, 망가진 파일을 엔진에 넘기기 전에 표식을 먼저 봅니다.
	const int32 SaveFileTag = 0x53415647;

	FString GetSlotName(int32 Slot)
	{
		return FString::Printf(TEXT("Slot%d"), Slot + 1);
	}

	// PIE의 월드 패키지 이름에는 PIE 접두사가 붙어 있어, 떼어 내야 스테이지 목록의 이름과 같습니다.
	FName GetDungeonKey(const UWorld& World)
	{
		return FName(*UWorld::RemovePIEPrefix(World.GetOutermost()->GetName()));
	}

	EAgSaveSlotState ReadSlot(int32 Slot, UAgSaveGame*& OutSaveGame)
	{
		OutSaveGame = nullptr;
		const FString SlotName = GetSlotName(Slot);
		if (!UGameplayStatics::DoesSaveGameExist(SlotName, 0))
		{
			return EAgSaveSlotState::Empty;
		}

		TArray<uint8> Data;
		int32 Tag = 0;
		if (UGameplayStatics::LoadDataFromSlot(Data, SlotName, 0) && Data.Num() >= static_cast<int32>(sizeof(Tag)))
		{
			FMemory::Memcpy(&Tag, Data.GetData(), sizeof(Tag));
			if (Tag == SaveFileTag)
			{
				OutSaveGame = Cast<UAgSaveGame>(UGameplayStatics::LoadGameFromMemory(Data));
			}
		}
		return OutSaveGame ? EAgSaveSlotState::Saved : EAgSaveSlotState::Corrupted;
	}
} // namespace AgSaveSubsystemPrivate

UAgSaveSubsystem& UAgSaveSubsystem::Get(const UObject* WorldContextObject)
{
	return *WorldContextObject->GetWorld()->GetGameInstance()->GetSubsystem<UAgSaveSubsystem>();
}

void UAgSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Current = CreateNewGame();
	PlayTimeStamp = FPlatformTime::Seconds();
}

void UAgSaveSubsystem::Deinitialize()
{
	// Quit 버튼과 창 닫기가 모두 이리로 옵니다.
	Save();

	Super::Deinitialize();
}

TArray<FAgSaveSlotInfo> UAgSaveSubsystem::ReadSlots() const
{
	TArray<FAgSaveSlotInfo> Slots;
	for (int32 Slot = 0; Slot < SlotCount; ++Slot)
	{
		UAgSaveGame* SaveGame = nullptr;
		FAgSaveSlotInfo& Info = Slots.AddDefaulted_GetRef();
		Info.State = AgSaveSubsystemPrivate::ReadSlot(Slot, SaveGame);
		if (SaveGame)
		{
			Info.LastStage = SaveGame->LastStage;
			Info.PlayTime = SaveGame->PlayTime;
			Info.SavedAt = SaveGame->SavedAt;
		}
	}
	return Slots;
}

void UAgSaveSubsystem::StartNewGame(int32 Slot)
{
	// 엔진은 슬롯 파일을 통째로 다시 쓰므로, 덮어쓸 때 옛 파일을 따로 지우지 않습니다.
	Current = CreateNewGame();
	CurrentSlot = Slot;
	PlayTimeStamp = FPlatformTime::Seconds();
	Save();
}

bool UAgSaveSubsystem::LoadSlot(int32 Slot)
{
	UAgSaveGame* SaveGame = nullptr;
	if (AgSaveSubsystemPrivate::ReadSlot(Slot, SaveGame) != EAgSaveSlotState::Saved)
	{
		UE_LOG(LogAg, Error, TEXT("Save: could not load %s."), *AgSaveSubsystemPrivate::GetSlotName(Slot));
		return false;
	}

	Current = SaveGame;
	CurrentSlot = Slot;
	PlayTimeStamp = FPlatformTime::Seconds();
	return true;
}

bool UAgSaveSubsystem::LoadLatestSlot()
{
	const TArray<FAgSaveSlotInfo> Slots = ReadSlots();
	int32 Latest = INDEX_NONE;
	for (int32 Slot = 0; Slot < Slots.Num(); ++Slot)
	{
		if (Slots[Slot].State == EAgSaveSlotState::Saved && (Latest == INDEX_NONE || Slots[Slot].SavedAt > Slots[Latest].SavedAt))
		{
			Latest = Slot;
		}
	}
	return Latest != INDEX_NONE && LoadSlot(Latest);
}

void UAgSaveSubsystem::DeleteSlot(int32 Slot)
{
	UGameplayStatics::DeleteGameInSlot(AgSaveSubsystemPrivate::GetSlotName(Slot), 0);
}

void UAgSaveSubsystem::Unload()
{
	Save();
	Current = CreateNewGame();
	CurrentSlot = INDEX_NONE;
}

const TMap<FName, int32>& UAgSaveSubsystem::GetItemCounts() const
{
	return Current->ItemCounts;
}

FName UAgSaveSubsystem::GetLastStage() const
{
	return Current->LastStage;
}

FName UAgSaveSubsystem::GetRestedCheckpoint(const UWorld& World) const
{
	const FAgDungeonProgress* Progress = Current->Dungeons.Find(AgSaveSubsystemPrivate::GetDungeonKey(World));
	return Progress ? Progress->Checkpoint : FName();
}

bool UAgSaveSubsystem::IsDeviceOpened(const AActor& Device) const
{
	const FAgDungeonProgress* Progress = Current->Dungeons.Find(AgSaveSubsystemPrivate::GetDungeonKey(*Device.GetWorld()));
	return Progress && Progress->OpenedDevices.Contains(Device.GetFName());
}

void UAgSaveSubsystem::SetItemCounts(const TMap<FName, int32>& Counts)
{
	if (!Current->ItemCounts.OrderIndependentCompareEqual(Counts))
	{
		Current->ItemCounts = Counts;
		Save();
	}
}

void UAgSaveSubsystem::SetLastStage(FName Stage)
{
	if (Current->LastStage != Stage)
	{
		Current->LastStage = Stage;
		Save();
	}
}

void UAgSaveSubsystem::SetRestedCheckpoint(const AActor& Checkpoint)
{
	FAgDungeonProgress& Progress = Current->Dungeons.FindOrAdd(AgSaveSubsystemPrivate::GetDungeonKey(*Checkpoint.GetWorld()));
	if (Progress.Checkpoint != Checkpoint.GetFName())
	{
		Progress.Checkpoint = Checkpoint.GetFName();
		Save();
	}
}

void UAgSaveSubsystem::AddOpenedDevice(const AActor& Device)
{
	Current->Dungeons.FindOrAdd(AgSaveSubsystemPrivate::GetDungeonKey(*Device.GetWorld())).OpenedDevices.Add(Device.GetFName());
	Save();
}

void UAgSaveSubsystem::ClearDungeonProgress(const UWorld& World)
{
	if (Current->Dungeons.Remove(AgSaveSubsystemPrivate::GetDungeonKey(World)) > 0)
	{
		Save();
	}
}

UAgSaveGame* UAgSaveSubsystem::CreateNewGame()
{
	UAgSaveGame* SaveGame = NewObject<UAgSaveGame>(this);
	for (const TPair<FName, FAgItemData>& Item : UAgItemSet::GetItems())
	{
		SaveGame->ItemCounts.Add(Item.Key, FMath::Clamp(Item.Value.StartCount, 0, Item.Value.MaxCount));
	}
	return SaveGame;
}

void UAgSaveSubsystem::Save()
{
	const double Now = FPlatformTime::Seconds();
	Current->PlayTime += Now - PlayTimeStamp;
	PlayTimeStamp = Now;
	if (CurrentSlot == INDEX_NONE)
	{
		return;
	}

	Current->SavedAt = FDateTime::Now();
	if (!UGameplayStatics::SaveGameToSlot(Current, AgSaveSubsystemPrivate::GetSlotName(CurrentSlot), 0))
	{
		UE_LOG(LogAg, Error, TEXT("Save: could not write %s."), *AgSaveSubsystemPrivate::GetSlotName(CurrentSlot));
	}
}
