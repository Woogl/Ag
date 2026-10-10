// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "AgGameplayEffect_TimedTag.generated.h"

/**
 * Gives the owner a tag for a time: cooldowns (쿨다운), regen delays (리젠 대기 시간) and the parry window (패리 구간).
 * The caller sets the duration on the spec and adds the tag to the spec's granted tags (UAgCombatLibrary::ApplyTimedTag).
 * Effect timers run in game time, so these keep counting during hitstop (전투 시스템 '히트스톱 중의 시간').
 */
UCLASS()
class UAgGameplayEffect_TimedTag : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UAgGameplayEffect_TimedTag();
};
