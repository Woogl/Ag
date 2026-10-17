// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask_SpawnActor.h"
#include "Data/AgMontageTimeline.h"
#include "Data/AgCharacterData.h"
#include "AgAbilityTask_SpawnProjectile.generated.h"
class AAgProjectile;

/** 시간표의 발사 항목마다 한 발. 권한 검사와 지연 생성은 엔진 SpawnActor를 사용합니다. */
UCLASS()
class UAgAbilityTask_SpawnProjectile : public UAbilityTask_SpawnActor
{
	GENERATED_BODY()

public:
	static UAgAbilityTask_SpawnProjectile* SpawnProjectile(UGameplayAbility* Owner, int32 MontageInstanceId,
	    const FAgProjectileDefinition& Projectile, const FAgAttackParams& Attack, EAgProjectileOrigin Origin, FName SocketName);
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
