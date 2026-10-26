// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/AgEnemyCharacter.h"
#include "AgBossCharacter.generated.h"

class AAgPlayerCharacter;
class UWidgetComponent;

UCLASS(Abstract)
class AAgBossCharacter : public AAgEnemyCharacter
{
	GENERATED_BODY()

public:
	AAgBossCharacter();

	virtual bool CanReturnToNonCombat() const override
	{
		return false;
	}

	FVector GetLockOnLocation() const;

	float GetKillSlowMotionSpeed() const
	{
		return KillSlowMotionSpeed;
	}

	float GetKillSlowMotionDuration() const
	{
		return KillSlowMotionDuration;
	}

protected:
	virtual void Tick(float DeltaSeconds) override;

private:
	bool IsExecutionTargetOf(AAgPlayerCharacter& Player) const;

	void UpdateWorldWidgets();

	UPROPERTY(VisibleAnywhere, Category = "UI")
	TObjectPtr<UWidgetComponent> LockOnMarker;

	UPROPERTY(VisibleAnywhere, Category = "UI")
	TObjectPtr<UWidgetComponent> ExecutionPrompt;

	UPROPERTY(EditDefaultsOnly, Category = "Death", meta = (ClampMin = "0.01", ClampMax = "1"))
	float KillSlowMotionSpeed = 0.3f;

	/** 실제 시간입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Death", meta = (ClampMin = "0"))
	float KillSlowMotionDuration = 1.f;
};
