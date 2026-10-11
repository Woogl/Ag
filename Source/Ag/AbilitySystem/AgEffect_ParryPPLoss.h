// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "AgEffect_ParryPPLoss.generated.h"

/**
 * 패리(플레이어 사양 '패리'): 막은 쪽이 공격한 쪽에 적용해, 공격한 쪽이 MaxPP의 일정 비율만큼 PP를 잃게 합니다.
 * 양은 UAgParryPPLossExecution이 정하고, 그로기 판정은 공격한 쪽의 어트리뷰트 세트가 합니다.
 */
UCLASS()
class UAgEffect_ParryPPLoss : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UAgEffect_ParryPPLoss();
};
