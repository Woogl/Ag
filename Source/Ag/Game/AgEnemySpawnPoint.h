// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TargetPoint.h"
#include "AgEnemySpawnPoint.generated.h"

UCLASS()
class AAgEnemySpawnPoint : public ATargetPoint
{
	GENERATED_BODY()

public:
	int32 GetSlot() const
	{
		return Slot;
	}

protected:
	UPROPERTY(EditInstanceOnly, Category = "SimulationRoom", meta = (ClampMin = 1))
	int32 Slot = 1;
};
