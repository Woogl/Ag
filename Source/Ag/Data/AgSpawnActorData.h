// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"
#include "Templates/SubclassOf.h"
#include "AgSpawnActorData.generated.h"

class AActor;

/** 생성 액터가 받는 속성값의 공통 부모입니다. 액터 클래스마다 하위 구조체 하나를 두고 IAgAbilitySpawnedActor로 알립니다. */
USTRUCT()
struct FAgSpawnActorData
{
	GENERATED_BODY()
};

/**
 * 어빌리티가 생성할 액터의 클래스와 속성값입니다. JSON에는 `SpawnActors.json`의 ID로 적고, 리더가 그 항목을 읽어 채웁니다.
 * 정의를 가진 데이터가 살아 있는 동안 클래스가 메모리에 남습니다.
 */
USTRUCT()
struct FAgSpawnActorDefinition
{
	GENERATED_BODY()

	static const TCHAR* GetFileName()
	{
		return TEXT("SpawnActors.json");
	}

	/** `SpawnActors.json`의 ID입니다. 오류에 적습니다. */
	UPROPERTY()
	FName Id;

	UPROPERTY()
	TSubclassOf<AActor> Class;

	/** `Class` 키를 뺀 속성값입니다. 그 액터 클래스가 정한 구조체로 읽어 두고, 생성할 때 그대로 넘깁니다. */
	UPROPERTY()
	TInstancedStruct<FAgSpawnActorData> Data;
};
