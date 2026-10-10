// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "AgGameplayEffect_Regen.generated.h"

/**
 * Regen (플레이어 사양 '자원 회복', 전투 시스템 'PP 리젠'): an endless periodic effect that adds the SetByCaller amount
 * every period. It pauses while its tag requirements fail, for example while the regen delay tag is on, and starts a
 * full period again when they pass. The caller sets the period and the amount per period from the data assets.
 */
UCLASS(Abstract)
class UAgGameplayEffect_Regen : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UAgGameplayEffect_Regen();

protected:
	/** Adds the regenerating attribute and the tags the owner must have and must not have for the regen to run. */
	void SetupRegen(const FGameplayAttribute& Attribute, const FGameplayTag& DataTag, const FGameplayTagContainer& RequiredTags, const FGameplayTagContainer& IgnoredTags);
};

/** SP regen while not guarding. */
UCLASS()
class UAgGameplayEffect_SPRegen : public UAgGameplayEffect_Regen
{
	GENERATED_BODY()

public:
	UAgGameplayEffect_SPRegen();
};

/** SP regen while guarding (가드 중 회복). */
UCLASS()
class UAgGameplayEffect_SPRegenGuarding : public UAgGameplayEffect_Regen
{
	GENERATED_BODY()

public:
	UAgGameplayEffect_SPRegenGuarding();
};

/** PP regen. */
UCLASS()
class UAgGameplayEffect_PPRegen : public UAgGameplayEffect_Regen
{
	GENERATED_BODY()

public:
	UAgGameplayEffect_PPRegen();
};
