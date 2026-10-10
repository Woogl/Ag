// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "AgCheatManager.generated.h"

/**
 * Test console commands. Cheats only exist in development builds, so players never see these.
 * They exist to reach states like 처형, 페이즈 전환 and the result UI quickly while testing.
 */
UCLASS()
class UAgCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	/** 플레이어를 사망시킵니다. */
	UFUNCTION(Exec)
	void AgKillPlayer();

	/** 보스를 사망시킵니다. */
	UFUNCTION(Exec)
	void AgKillBoss();
};
