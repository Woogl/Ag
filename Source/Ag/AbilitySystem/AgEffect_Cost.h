// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "AgEffect_Cost.generated.h"

/**
 * 행동 비용(플레이어 사양 '자원'): 비용을 치르는 어빌리티의 비용 GE(CostGameplayEffectClass)로, CommitAbility가 적용합니다.
 * 줄이는 양은 UAgCostExecution이 그 어빌리티의 GetCost에서 읽습니다.
 */
UCLASS()
class UAgEffect_Cost : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UAgEffect_Cost();
};
