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
};
