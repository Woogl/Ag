// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "AgEffect_Resource.generated.h"

/**
 * 자원(SP·MP·UP)을 얻습니다: MP·UP 충전(적중, 패리, 극한 회피)과 극한 회피의 SP 반환. 얻는 양은 줄 때마다 달라
 * SetByCaller(Data.SP, Data.MP, Data.UP)로 받습니다.
 */
UCLASS()
class UAgEffect_Resource : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UAgEffect_Resource();

	/** 얻는 SP·MP·UP를 스펙에 담습니다. */
	static void SetAmounts(FGameplayEffectSpec& Spec, float SP, float MP, float UP);
};
