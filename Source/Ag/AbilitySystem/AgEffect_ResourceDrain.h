// Copyright Woogle. All Rights Reserved.

#pragma once

#include "AbilitySystem/AgEffect_Resource.h"
#include "AgEffect_ResourceDrain.generated.h"

/** 소유 Ability가 제거하는 주기 소모. 금액은 실행 시 Spec에 설정합니다. */
UCLASS()
class UAgEffect_ResourceDrain : public UAgEffect_Resource
{
	GENERATED_BODY()

public:
	UAgEffect_ResourceDrain();
};
