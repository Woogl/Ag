// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "AgParryPPLossExecution.generated.h"

/**
 * 패리로 공격한 쪽이 잃는 PP: 대상(공격한 쪽)의 MaxPP에 막은 쪽(GE 컨텍스트의 Instigator)의 패리 PP 비율을 곱해 반올림한
 * 만큼 줄입니다. PP를 잃을 수 없는 대상(그로기 면역, 그로기)은 그대로입니다.
 */
UCLASS()
class UAgParryPPLossExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UAgParryPPLossExecution();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
