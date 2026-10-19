// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Game/AgGameMode.h"
#include "AgFrontEndGameMode.generated.h"

class UAgTitleScreen;

UCLASS(Abstract)
class AAgFrontEndGameMode : public AAgGameMode
{
	GENERATED_BODY()

public:
	AAgFrontEndGameMode();

	void OpenBossStage();

protected:
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgTitleScreen> TitleScreenClass;

	UPROPERTY(EditDefaultsOnly, Category = "Maps")
	TSoftObjectPtr<UWorld> BossStageMap;
};
