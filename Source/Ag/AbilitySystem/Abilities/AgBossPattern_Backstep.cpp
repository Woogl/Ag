// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgBossPattern_Backstep.h"

#include "Character/AgBossCharacter.h"

float UAgBossPattern_Backstep::GetPatternPlayRate() const
{
	const AAgBossCharacter* Boss = GetBoss();
	return Boss ? Boss->GetPatternSpeed() : 1.f;
}
