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

protected:
	virtual void Tick(float DeltaSeconds) override;

private:
	bool IsExecutionTargetOf(AAgPlayerCharacter& Player) const;

	void UpdateWorldWidgets();

	UPROPERTY(VisibleAnywhere, Category = "UI")
	TObjectPtr<UWidgetComponent> LockOnMarker;

	UPROPERTY(VisibleAnywhere, Category = "UI")
	TObjectPtr<UWidgetComponent> ExecutionPrompt;
};
