// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "AgGameplayEffect_Resource.generated.h"

/**
 * Instant change of SP, MP and UP: action costs, MP·UP 충전, the guard's SP use and the 극한 회피 refund.
 * The amounts come by SetByCaller (Data.SP, Data.MP, Data.UP), so the effect holds no data of its own.
 */
UCLASS()
class UAgGameplayEffect_Resource : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UAgGameplayEffect_Resource();
};
