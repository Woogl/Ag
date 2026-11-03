// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Game/AgGameMode.h"
#include "AgSimulationRoomGameMode.generated.h"

class AAgCharacterBase;
class AAgCharacterPreview;
class AAgEnemySpawnPoint;
class UAgEnemySelectScreen;
class UTextureRenderTarget2D;

UCLASS(Abstract)
class AAgSimulationRoomGameMode : public AAgGameMode
{
	GENERATED_BODY()

public:
	const TArray<TSubclassOf<AAgCharacterBase>>& GetEnemyClasses() const
	{
		return EnemyClasses;
	}

	/** 후보 가운데 그 캐릭터 데이터 ID의 클래스입니다. 없으면 nullptr입니다. */
	TSubclassOf<AAgCharacterBase> FindEnemyClass(FName CharacterDataId) const;

	int32 GetSlotCount() const
	{
		return SpawnPoints.Num();
	}

	/** 자리별 구성(빈 자리는 nullptr)에서 그 자리에 그 적을 둘 수 있는지 정합니다. 보스는 다른 자리에 보스가 없어야 합니다. */
	bool CanPlace(const TArray<TSubclassOf<AAgCharacterBase>>& InLineup, int32 SlotIndex, TSubclassOf<AAgCharacterBase> EnemyClass) const;

	void StartLineup(const TArray<TSubclassOf<AAgCharacterBase>>& NewLineup);

	/** 후보 적의 모습을 그리는 렌더 타깃입니다. 처음 부를 때 그 적의 표시 액터를 띄웁니다. 후보가 아니면 nullptr입니다. */
	UTextureRenderTarget2D* GetEnemyPreview(TSubclassOf<AAgCharacterBase> EnemyClass);

protected:
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual bool IsStageCleared(const AAgCharacterBase& DeadCharacter) const override;

	UPROPERTY(EditDefaultsOnly, Category = "SimulationRoom")
	TArray<TSubclassOf<AAgCharacterBase>> EnemyClasses;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgEnemySelectScreen> EnemySelectScreenClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<AAgCharacterPreview> EnemyPreviewClass;

private:
	void CollectSpawnPoints();
	void SpawnLineup(AActor& Player);

	/** 자리 번호 순서이며 빈 자리는 nullptr입니다. */
	TArray<TSubclassOf<AAgCharacterBase>> Lineup;

	UPROPERTY()
	TArray<TObjectPtr<AAgEnemySpawnPoint>> SpawnPoints;

	UPROPERTY()
	TMap<TSubclassOf<AAgCharacterBase>, TObjectPtr<AAgCharacterPreview>> EnemyPreviews;
};
