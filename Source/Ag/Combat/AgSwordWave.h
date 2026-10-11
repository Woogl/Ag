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
 * A6 검기 (보스 사양 'A6', '검기'): flies along the ground in a straight line and judges the player once, then flies on
 * through. It vanishes when it touches a wall, reaches its range, or its shooter dies; other changes to the shooter don't
 * stop it. The hit box size (폭, 높이) and the look (mesh, material) are set on the blueprint; speed and range come at
 * launch.
 */
UCLASS(Abstract)
class AAgSwordWave : public AActor
{
	GENERATED_BODY()

public:
	AAgSwordWave();

	/** Direction(수평) 쪽으로 검기를 날립니다. 쏜 캐릭터는 스폰할 때 넣은 Instigator입니다. 바닥에 스폰되며, 상자를 바닥 위로 올립니다. */
	void Launch(const FAgAttackHit& InHit, const FVector& Direction, float InSpeed, float InRange);

protected:
	virtual void Tick(float DeltaSeconds) override;

private:
	/** The attack box, its bottom on the ground. Only used for queries. X is the depth along the flight, Y the width. */
	UPROPERTY(VisibleAnywhere, Category = "Sword Wave")
	TObjectPtr<UBoxComponent> HitBox;

	/** The look, placed over the box on the blueprint. */
	UPROPERTY(VisibleAnywhere, Category = "Sword Wave")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Characters the wave already reached; each is judged once. */
	TArray<TWeakObjectPtr<AActor>> TouchedActors;

	FAgAttackHit Hit;
	FVector FlightDirection = FVector::ForwardVector;
	float Speed = 0.f;
	float Range = 0.f;
	float Traveled = 0.f;
};
