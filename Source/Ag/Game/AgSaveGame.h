// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "AgSaveGame.generated.h"

USTRUCT()
struct FAgDungeonProgress
{
	GENERATED_BODY()

	/** 마지막으로 휴식한 체크포인트 액터의 이름입니다. */
	UPROPERTY()
	FName Checkpoint;

	/** 연 장치(성문) 액터의 이름입니다. */
	UPROPERTY()
	TSet<FName> OpenedDevices;
};

/** 저장 슬롯 하나의 내용입니다. 읽고 쓰는 일은 UAgSaveSubsystem이 맡습니다. */
UCLASS()
class UAgSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TMap<FName, int32> ItemCounts;

	/** 키는 던전 레벨의 패키지 이름입니다. */
	UPROPERTY()
	TMap<FName, FAgDungeonProgress> Dungeons;

	/** 스테이지 선택 화면에서 마지막으로 들어간 레벨의 패키지 이름입니다. */
	UPROPERTY()
	FName LastStage;

	/** 초 단위입니다. */
	UPROPERTY()
	double PlayTime = 0.0;

	UPROPERTY()
	FDateTime SavedAt;
};
