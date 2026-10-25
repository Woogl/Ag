// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "AgGameState.generated.h"

class AAgBossCharacter;

UCLASS()
class AAgGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AAgBossCharacter* GetBoss() const
	{
		return Boss.Get();
	}

	void SetBoss(AAgBossCharacter* InBoss);

	FSimpleMulticastDelegate OnBossChanged;

private:
	TWeakObjectPtr<AAgBossCharacter> Boss;
};
