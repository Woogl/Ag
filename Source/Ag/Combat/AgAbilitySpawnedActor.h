// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AgAbilitySpawnedActor.generated.h"

class UAgGameplayAbility;

UINTERFACE()
class UAgAbilitySpawnedActor : public UInterface
{
	GENERATED_BODY()
};

/** 어빌리티가 생성한 액터가 생성을 마치기 전에 그 어빌리티를 받습니다. */
class IAgAbilitySpawnedActor
{
	GENERATED_BODY()

public:
	virtual void OnSpawnedByAbility(const UAgGameplayAbility& Ability) = 0;
};
