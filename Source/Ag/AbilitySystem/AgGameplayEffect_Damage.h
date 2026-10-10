// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "AgGameplayEffect_Damage.generated.h"

/** Applies an attack's HP and PP change, passed as SetByCaller Data.HP and Data.PP (negative values reduce). */
UCLASS()
class UAgGameplayEffect_Damage : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UAgGameplayEffect_Damage();
};
