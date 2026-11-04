// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_SpawnActor.generated.h"

USTRUCT()
struct FAgSpawnActorParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY(meta = (DataId = "SpawnActors"))
	FName Actor;

	UPROPERTY()
	FName SocketName;

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

// 엔진 UAbilityTask_SpawnActor는 블루프린트 노드용이라 생성이 두 호출로 나뉘므로, 생성은 Activate에서 직접 지연 생성합니다.
UCLASS()
class UAgAbilityTask_SpawnActor : public UAbilityTask
{
	GENERATED_BODY()
	friend struct FAgSpawnActorParams;

public:
	virtual void Activate() override;

private:
	UPROPERTY()
	FAgSpawnActorParams Params;
};
