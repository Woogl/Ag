// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask_SpawnActor.h"
#include "AbilitySystem/Tasks/AgAbilityTask_Attack.h"
#include "Data/AgProjectileData.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_SpawnProjectile.generated.h"
class AAgProjectile;

/** 투사체를 쏘는 위치 */
UENUM()
enum class EAgProjectileOrigin : uint8
{
	CharacterFeetForward,
	MeshSocket
};

/** 투사체 한 발(SpawnProjectile Task의 Params). Attack은 그 투사체의 공격 데이터이고, 판정 범위(Radius, bGroundOnly)는 쓰지 않습니다. */
USTRUCT()
struct FAgSpawnProjectileParams : public FAgTaskParams
{
	GENERATED_BODY()

	/** Projectiles.json의 투사체 ID */
	UPROPERTY()
	FName ProjectileId;

	UPROPERTY()
	EAgProjectileOrigin Origin = EAgProjectileOrigin::CharacterFeetForward;

	/** Origin이 MeshSocket일 때만 적습니다. */
	UPROPERTY()
	FName SocketName;

	UPROPERTY()
	FAgAttackParams Attack;

	virtual EAgTaskPlacement GetPlacements() const override
	{
		return EAgTaskPlacement::Point;
	}

	virtual void GetFieldRules(FAgJsonFieldRules& Rules) const override;
	virtual void Validate(const FJsonObject& Json, EAgTaskPlacement Placement, const FString& Path, TArray<FString>& Errors) override;
	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

/** 시간표의 발사 항목마다 한 발. 권한 검사와 지연 생성은 엔진 SpawnActor를 사용합니다. */
UCLASS()
class UAgAbilityTask_SpawnProjectile : public UAbilityTask_SpawnActor
{
	GENERATED_BODY()

public:
	static UAgAbilityTask_SpawnProjectile* SpawnProjectile(UGameplayAbility* Owner, int32 MontageInstanceId,
	                                                       const FAgProjectileDefinition& Projectile, const FAgAttackParams& Attack,
	                                                       EAgProjectileOrigin Origin, FName SocketName);
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
