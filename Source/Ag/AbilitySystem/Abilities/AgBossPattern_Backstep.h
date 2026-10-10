// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AgBossPatternAbility.h"
#include "AgBossPattern_Backstep.generated.h"

/**
 * B1 백스텝: steps back by the motion's own root motion. Not an attack pattern, so it plays at its own speed without the
 * 모션 재생 속도 or the phase 2 speed-up, and keeps no stop distance. The AI starts it on a player's basic attack.
 */
UCLASS()
class UAgBossPattern_Backstep : public UAgBossPatternAbility
{
	GENERATED_BODY()

protected:
	virtual float GetPatternPlayRate() const override { return 1.f; }
	virtual bool UsesSharedMovement() const override { return false; }
};
