// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "AgEffect_Resource.generated.h"

/**
 * SP·MP·UP를 즉시 증감합니다. 변화량은 Spec의 SetByCaller로 받으며, 회복은 양수·행동 비용은 음수입니다.
 */
UCLASS()
class UAgEffect_Resource : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UAgEffect_Resource();

	/** SP·MP·UP의 변화량을 스펙에 담습니다. */
	static void SetAmounts(FGameplayEffectSpec& Spec, float SP, float MP, float UP);
};
