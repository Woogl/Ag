// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgHitReward.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_SpawnProjectile.generated.h"

USTRUCT()
struct FAgSpawnProjectileParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY(meta = (DataId = "Projectiles"))
	FName Projectile;

	UPROPERTY()
	FName SocketName;

	UPROPERTY()
	TOptional<FAgHitReward> HitReward;

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

// 엔진 UAbilityTask_SpawnActor는 블루프린트 노드용이라 생성이 두 호출로 나뉘므로, 발사는 Activate에서 직접 지연 생성합니다.
UCLASS()
class UAgAbilityTask_SpawnProjectile : public UAbilityTask
{
	GENERATED_BODY()
	friend struct FAgSpawnProjectileParams;

public:
	virtual void Activate() override;

private:
	UPROPERTY()
	FAgSpawnProjectileParams Params;
};
