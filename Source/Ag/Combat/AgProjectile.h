// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/AgAttackData.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "AgProjectile.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class AAgCharacterBase;

UCLASS(Abstract)
class AAgProjectile : public AActor
{
	GENERATED_BODY()

public:
	AAgProjectile();

	void Launch(const FGameplayEffectSpecHandle& InDamageSpec, const FVector& Direction, const FAgProjectileParams& InParams);
	float GetCollisionHalfHeight() const;

protected:
	virtual void Tick(float DeltaSeconds) override;

private:
	void ApplyHit(AAgCharacterBase& Target, const FVector& HitLocation);

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UBoxComponent> HitBox;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> Mesh;

	TArray<TWeakObjectPtr<AActor>> TouchedActors;

	/** 여러 대상에 다시 쓰므로, 대상별 결과와 적중 위치는 이 스펙의 복사본에만 기록합니다. */
	UPROPERTY()
	FGameplayEffectSpecHandle DamageSpec;
	FAgProjectileParams Params;
	FVector FlightDirection = FVector::ForwardVector;
	float Traveled = 0.f;
};
