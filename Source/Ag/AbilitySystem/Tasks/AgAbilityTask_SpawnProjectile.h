// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask_SpawnActor.h"
#include "Data/AgProjectileData.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_SpawnProjectile.generated.h"
class AAgProjectile;

UENUM()
enum class EAgProjectileOrigin : uint8
{
	CharacterFeetForward,
	MeshSocket
};

USTRUCT()
struct FAgSpawnProjectileParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY(meta = (DataId = "Projectiles"))
	FName ProjectileId;

	UPROPERTY()
	EAgProjectileOrigin Origin = EAgProjectileOrigin::CharacterFeetForward;

	UPROPERTY()
	FName SocketName;

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

UCLASS()
class UAgAbilityTask_SpawnProjectile : public UAbilityTask_SpawnActor
{
	GENERATED_BODY()

public:
	static UAgAbilityTask_SpawnProjectile* SpawnProjectile(UGameplayAbility* Owner, int32 MontageInstanceId, const FAgProjectileDefinition& Projectile, EAgProjectileOrigin Origin, FName SocketName);
	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	UFUNCTION()
	void HandleSpawned(AActor* Actor);
	bool IsSourceActive() const;
	int32 SourceInstanceId = INDEX_NONE;

	UPROPERTY()
	TSubclassOf<AAgProjectile> ProjectileClass;
	EAgProjectileOrigin SpawnOrigin = EAgProjectileOrigin::CharacterFeetForward;
	FName SpawnSocket;
	FAgPlayerAttackHit Hit;
	FAgProjectileParams FlightParams;
	FVector Direction = FVector::ForwardVector;

	UPROPERTY()
	TObjectPtr<AAgProjectile> PendingProjectile;
};
