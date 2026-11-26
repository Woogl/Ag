// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AgSaveSubsystem.generated.h"

class UAgSaveGame;

enum class EAgSaveSlotState : uint8
{
	Empty,
	Saved,
	Corrupted
};

struct FAgSaveSlotInfo
{
	EAgSaveSlotState State = EAgSaveSlotState::Empty;
	FName LastStage;
	double PlayTime = 0.0;
	FDateTime SavedAt;
};

/**
 * 저장 슬롯과 지금 진행의 주인입니다. 지금 진행은 언제나 하나 있고, 슬롯을 불러오지 않았으면(에디터에서 스테이지를 바로
 * 플레이할 때) 시작 보유 수로 만든 진행이며 파일로 쓰지 않습니다. 바꾸는 함수는 값을 바꾼 직후 바로 저장합니다.
 */
UCLASS()
class UAgSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static constexpr int32 SlotCount = 6;

	static UAgSaveSubsystem& Get(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** 슬롯 파일을 모두 읽어 슬롯 번호순으로 돌려줍니다. */
	TArray<FAgSaveSlotInfo> ReadSlots() const;

	/** 빈 슬롯 중 번호가 가장 작은 슬롯에 새 게임을 만들어 저장합니다. 빈 슬롯이 없으면 false입니다. */
	bool StartNewGame();

	bool LoadSlot(int32 Slot);

	/** 저장한 시각이 가장 늦은 슬롯을 불러옵니다. 저장된 슬롯이 없으면 false입니다. */
	bool LoadLatestSlot();

	void DeleteSlot(int32 Slot);

	/** 저장하고 슬롯을 내려놓습니다. 지금 진행은 슬롯 없는 새 진행이 됩니다. */
	void Unload();

	const TMap<FName, int32>& GetItemCounts() const;

	FName GetLastStage() const;

	/** 그 월드의 던전에서 마지막으로 휴식한 체크포인트 액터의 이름입니다. 없으면 NAME_None입니다. */
	FName GetRestedCheckpoint(const UWorld& World) const;

	bool IsDeviceOpened(const AActor& Device) const;

	void SetItemCounts(const TMap<FName, int32>& Counts);

	void SetLastStage(FName Stage);

	void SetRestedCheckpoint(const AActor& Checkpoint);

	void AddOpenedDevice(const AActor& Device);

	/** 그 월드의 던전 진행(휴식한 체크포인트, 연 장치)을 지웁니다. */
	void ClearDungeonProgress(const UWorld& World);

private:
	UAgSaveGame* CreateNewGame();

	/** 플레이 시간을 더하고, 슬롯을 불러와 있으면 파일로 씁니다. */
	void Save();

	UPROPERTY()
	TObjectPtr<UAgSaveGame> Current;

	/** 불러온 슬롯이 없으면 INDEX_NONE입니다. */
	int32 CurrentSlot = INDEX_NONE;

	/** 플레이 시간을 마지막으로 더한 시각(FPlatformTime::Seconds)입니다. */
	double PlayTimeStamp = 0.0;
};
