// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/AgEnemyCharacter.h"
#include "AgBossCharacter.generated.h"

UCLASS(Abstract)
class AAgBossCharacter : public AAgEnemyCharacter
{
	GENERATED_BODY()

public:
	virtual bool CanReturnToNonCombat() const override
	{
		return false;
	}

	float GetKillSlowMotionSpeed() const
	{
		return KillSlowMotionSpeed;
	}

	float GetKillSlowMotionDuration() const
	{
		return KillSlowMotionDuration;
	}

private:
	UPROPERTY(EditDefaultsOnly, Category = "Death", meta = (ClampMin = "0.01", ClampMax = "1"))
	float KillSlowMotionSpeed = 0.3f;

	/** 실제 시간입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Death", meta = (ClampMin = "0"))
	float KillSlowMotionDuration = 1.f;
};
