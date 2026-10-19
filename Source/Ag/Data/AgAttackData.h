// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AgAttackData.generated.h"

struct FGameplayEffectSpec;

UENUM(BlueprintType)
enum class EAgHitReaction : uint8
{
	None UMETA(DisplayName = "없음"),
	Flinch UMETA(DisplayName = "움찔"),
	KnockBack UMETA(DisplayName = "넉백"),
	KnockUp UMETA(DisplayName = "넉업"),
};

USTRUCT(BlueprintType)
struct FAgAttackHit
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = 0))
	float DamageMultiplier = 1.f;

	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = 0))
	float PoiseMultiplier = 1.f;

	UPROPERTY(EditAnywhere, Category = "Attack")
	EAgHitReaction HitReaction = EAgHitReaction::Flinch;

	UPROPERTY(EditAnywhere, Category = "Attack")
	bool bGuardable = true;
};

USTRUCT(BlueprintType)
struct FAgPlayerAttackHit : public FAgAttackHit
{
	GENERATED_BODY()

	FAgPlayerAttackHit() = default;

	explicit FAgPlayerAttackHit(const FAgAttackHit& Hit)
		: FAgAttackHit(Hit)
	{
	}

	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = 0))
	float MPCharge = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = 0))
	float UPCharge = 0.f;

	bool bFinisher = false;

	void AddToSpec(FGameplayEffectSpec& Spec) const;
};

USTRUCT(BlueprintType)
struct FAgProjectileParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Projectile", meta = (Units = "cm/s", ClampMin = 0))
	float Speed = 0.f;

	UPROPERTY(EditAnywhere, Category = "Projectile", meta = (Units = "cm", ClampMin = 0))
	float Range = 0.f;
};
