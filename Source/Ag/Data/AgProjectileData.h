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

	/** 이 투사체가 맞힌 대상에게 주는 공격 값(공격 데이터 1타) */
	UPROPERTY()
	FAgPlayerAttackHit Attack;
};

/** Projectiles.json의 값: 투사체 ID마다 클래스, 비행 값, 공격 값(구현 계획서 'JSON 데이터'). 몽타주 시간표의 발사 항목이 ID로 가리킵니다. */
UCLASS()
class UAgProjectileSet : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, VisibleAnywhere, Category = "Projectiles")
	TMap<FName, FAgProjectileDefinition> Projectiles;
};
