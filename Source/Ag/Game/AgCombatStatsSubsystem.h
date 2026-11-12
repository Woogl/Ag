// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AgCombatStatsSubsystem.generated.h"

UENUM()
enum class EAgCombatResult : uint8
{
	Victory,
	Death,
	Disengage
};

USTRUCT()
struct FAgCombatEnemyStats
{
	GENERATED_BODY()

	/** 캐릭터 데이터 ID입니다. */
	UPROPERTY()
	FName Id;

	/** 레벨에 처음 놓인 자리입니다. */
	UPROPERTY()
	FIntVector Location = FIntVector::ZeroValue;
};

/** 전투 한 번의 통계입니다. 속성 이름이 기록 파일의 키입니다. */
USTRUCT()
struct FAgCombatStats
{
	GENERATED_BODY()

	UPROPERTY()
	FString Level;

	UPROPERTY()
	FString EndedAt;

	UPROPERTY()
	int32 Attempt = 1;

	UPROPERTY()
	EAgCombatResult Result = EAgCombatResult::Victory;

	/** 실제 시간(초)입니다. 파일에는 밀리초까지 씁니다. */
	UPROPERTY()
	float Duration = 0.f;

	UPROPERTY()
	TArray<FAgCombatEnemyStats> Enemies;

	UPROPERTY()
	int32 ParryCount = 0;

	UPROPERTY()
	int32 PerfectDodgeCount = 0;

	UPROPERTY()
	int32 HitCount = 0;
};

USTRUCT()
struct FAgCombatStatsFile
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FAgCombatStats> Records;
};

/** 레벨을 다시 열어도 남아야 하는 도전 횟수와, 이번 실행에서 끝난 전투 통계의 기록 파일을 맡습니다. */
UCLASS()
class UAgCombatStatsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Restart로 같은 레벨을 다시 열기 직전에 부릅니다. */
	void RequestRestart();

	/** 레벨을 열 때 게임 모드가 부릅니다. Restart 요청이 있었으면 도전 횟수에 1을 더하고, 아니면 1로 돌아갑니다. */
	void BeginAttempt();

	int32 GetAttempt() const
	{
		return Attempt;
	}

	/** 기록을 덧붙이고 파일 전체를 다시 씁니다. */
	void Save(const FAgCombatStats& Stats);

private:
	FString FilePath;

	FAgCombatStatsFile File;

	int32 Attempt = 1;

	bool bRestartRequested = false;
};
