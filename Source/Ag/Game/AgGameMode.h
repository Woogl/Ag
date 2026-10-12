// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AgGameMode.generated.h"

/**
 * 타이틀용·보스전용 게임 모드의 부모. 타이틀로 돌아가기와 게임 종료를 맡습니다(게임 플로우).
 * 보스전을 여는 것은 타이틀(AAgFrontEndGameMode), 다시 시작하는 것은 보스전(AAgBossStageGameMode)이 맡습니다.
 */
UCLASS(Abstract)
class AAgGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AAgGameMode();

	/** 타이틀 화면 레벨(엔진의 Game Default Map, LV_FrontEnd)로 돌아갑니다. */
	void OpenFrontEnd();

	void QuitGame();
};
