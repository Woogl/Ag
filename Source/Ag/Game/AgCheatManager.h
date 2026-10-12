// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "AgCheatManager.generated.h"

/** 테스트용 콘솔 명령. 치트는 개발 빌드에만 있어 플레이어에게 보이지 않습니다. 결과 UI를 빠르게 확인할 때 씁니다. */
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
