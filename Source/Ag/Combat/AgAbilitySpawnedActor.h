// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AgAbilitySpawnedActor.generated.h"

class UAgGameplayAbility;
struct FAgSpawnActorData;

UINTERFACE()
class UAgAbilitySpawnedActor : public UInterface
{
	GENERATED_BODY()
};

/** 어빌리티가 생성한 액터가 생성을 마치기 전에 그 어빌리티와 `SpawnActors.json`의 속성값을 받습니다. */
class IAgAbilitySpawnedActor
{
	GENERATED_BODY()

public:
	/** 이 액터가 받는 속성값의 구조체(FAgSpawnActorData의 하위)입니다. 데이터를 읽을 때 클래스 기본 객체에 묻습니다. */
	virtual const UScriptStruct& GetSpawnDataType() const = 0;

	/** Data는 GetSpawnDataType이 돌려준 구조체입니다. */
	virtual void OnSpawnedByAbility(const UAgGameplayAbility& Ability, const FAgSpawnActorData& Data) = 0;
};
