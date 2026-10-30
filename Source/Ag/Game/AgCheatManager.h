// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "AgCheatManager.generated.h"

UCLASS()
class UAgCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	UFUNCTION(Exec)
	void AgKillPlayer();

	UFUNCTION(Exec)
	void AgKillBoss();

	UFUNCTION(Exec)
	void AgKillEnemies();

	UFUNCTION(Exec)
	void AgGod();

	UFUNCTION(Exec)
	void AgFillMPUP();

	UFUNCTION(Exec)
	void AgGroggy();

	UFUNCTION(Exec)
	void AgPattern(FName Id);

	UFUNCTION(Exec)
	void AgSetHP(float Percent);

	UFUNCTION(Exec)
	void AgStopAI();

	UFUNCTION(Exec)
	void AgStartAI();

private:
	bool bGod = false;
};
