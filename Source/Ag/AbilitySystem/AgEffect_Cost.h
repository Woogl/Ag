// Copyright Woogle. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgEffect_Resource.h"
#include "AgEffect_Cost.generated.h"

struct FAgResourceCostParams;

/** 커밋 시 전달받은 비용을 즉시 자원 Modifier로 적용합니다. 실행 값은 Spec에만 보관합니다. */
UCLASS()
class UAgEffect_Cost : public UAgEffect_Resource
{
	GENERATED_BODY()
public:
	static void SetCost(FGameplayEffectSpec& Spec, const FAgResourceCostParams& Cost);
};
