// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/AgAttackData.h"
#include "GameFramework/Actor.h"
#include "AgSwordWave.generated.h"

class AAgCharacterBase;
class UBoxComponent;
class UStaticMeshComponent;

/**
 * A6 검기 (보스 사양 'A6', '검기'): flies along the ground in a straight line and hits the player once.
 * It vanishes when it hits, is guarded or parried, touches a wall, reaches its range, or its shooter dies; other changes
 * to the shooter don't stop it. The look (mesh, material) is set on the blueprint; size, speed and range come at launch.
 */
UCLASS(Abstract)
class AAgSwordWave : public AActor
{
	GENERATED_BODY()

public:
	AAgSwordWave();

	/**
	 * Starts the wave from Location toward Direction (horizontal). Size is the hit box (depth along the flight, width,
	 * height from the ground); Location is on the ground.
	 */
	void Launch(AAgCharacterBase* InShooter, const FAgAttackHit& InHit, const FVector& Direction, float InSpeed, float InRange, const FVector& Size);

protected:
	virtual void Tick(float DeltaSeconds) override;

private:
	/** The attack box, its bottom on the ground. Only used for queries. */
	UPROPERTY(VisibleAnywhere, Category = "Sword Wave")
	TObjectPtr<UBoxComponent> HitBox;

	/** The look, fitted to the box size at launch. */
	UPROPERTY(VisibleAnywhere, Category = "Sword Wave")
	TObjectPtr<UStaticMeshComponent> Mesh;

	TWeakObjectPtr<AAgCharacterBase> Shooter;
	FAgAttackHit Hit;
	FVector FlightDirection = FVector::ForwardVector;
	float Speed = 0.f;
	float Range = 0.f;
	float Traveled = 0.f;
};
