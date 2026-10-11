// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "AgRegenCalculation.generated.h"

/**
 * 리젠 GE의 한 주기 양: 그 리젠의 초당 회복량(주인의 캐릭터 데이터)에 스펙의 주기를 곱합니다. 주기마다 다시 계산하므로
 * 데이터 파일을 다시 읽으면 다음 주기부터 바뀝니다.
 */
UCLASS()
class UAgRegenCalculation : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
