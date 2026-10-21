// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AgHitPhysicsComponent.generated.h"

class USkeletalMeshComponent;
struct FGameplayTag;

struct FAgPhysicsHit
{
	FVector Location = FVector::ZeroVector;

	FVector Direction = FVector::ZeroVector;

	FName Bone;

	bool bGuard = false;
};

UCLASS()
class UAgHitPhysicsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAgHitPhysicsComponent();

	void Play(const FAgPhysicsHit& Hit);
	void Stop();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	struct FAgFrozenBody
	{
		int32 BodyIndex = INDEX_NONE;
		FVector LinearVelocity = FVector::ZeroVector;
		FVector AngularVelocity = FVector::ZeroVector;
		bool bGravity = true;
	};

	void Start(const FAgPhysicsHit& Hit);

	void HandleHitstopChanged(const FGameplayTag Tag, int32 NewCount);

	void ResumeBodies();

	USkeletalMeshComponent* GetMesh() const;

	UPROPERTY(EditDefaultsOnly, Category = "Hit Physics")
	float PushSpeed = 500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Hit Physics", meta = (ClampMin = 0, ClampMax = 1))
	float MaxBlendWeight = 0.7f;

	TArray<FAgPhysicsHit> PendingHits;

	TArray<FAgFrozenBody> FrozenBodies;

	FName RangeBone;

	bool bArms = false;

	float Weight = 0.f;

	bool bRising = false;

	bool bHitstop = false;
};
