// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask_SpawnActor.h"
#include "Data/AgProjectileData.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_SpawnProjectile.generated.h"

class AAgProjectile;

USTRUCT()
struct FAgSpawnProjectileParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY(meta = (DataId = "Projectiles"))
	FName ProjectileId;

	UPROPERTY()
	FName SocketName;

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

UCLASS()
class UAgAbilityTask_SpawnProjectile : public UAbilityTask_SpawnActor
{
	GENERATED_BODY()

public:
	static UAgAbilityTask_SpawnProjectile* SpawnProjectile(UGameplayAbility* Owner, const FAgProjectileDefinition& Projectile, FName SocketName);
	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	UFUNCTION()
	void HandleSpawned(AActor* Actor);

	UPROPERTY()
	TSubclassOf<AAgProjectile> ProjectileClass;
	FName SpawnSocket;
	FAgDamage Damage;
	FAgProjectileParams FlightParams;
	FVector Direction = FVector::ForwardVector;

	UPROPERTY()
	TObjectPtr<AAgProjectile> PendingProjectile;
};
