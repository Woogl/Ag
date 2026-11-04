// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AgSpawnActorData.generated.h"

UCLASS()
class UAgSpawnActorSet : public UObject
{
	GENERATED_BODY()

public:
	/** ID마다 어빌리티가 생성할 액터의 템플릿(클래스와 속성값)입니다. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "SpawnActors")
	TMap<FName, TObjectPtr<AActor>> SpawnActors;
};
