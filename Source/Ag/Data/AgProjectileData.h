// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/AgAttackData.h"
#include "AgProjectileData.generated.h"

class AAgProjectile;

USTRUCT()
struct FAgProjectileDefinition
{
	GENERATED_BODY()

	UPROPERTY()
	TSubclassOf<AAgProjectile> ProjectileClass;

	UPROPERTY()
	FAgProjectileParams Params;

	UPROPERTY()
	FAgDamage Damage;
};

UCLASS()
class UAgProjectileSet : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, VisibleAnywhere, Category = "Projectiles")
	TMap<FName, FAgProjectileDefinition> Projectiles;
};
