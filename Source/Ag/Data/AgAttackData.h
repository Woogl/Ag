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

USTRUCT()
struct FAgDamage
{
	GENERATED_BODY()

	UPROPERTY()
	float HPMultiplier = 0.f;

	UPROPERTY()
	float PPMultiplier = 0.f;

	UPROPERTY()
	EAgHitReaction HitReaction = EAgHitReaction::None;

	UPROPERTY()
	bool bGuardable = true;

	UPROPERTY()
	float MPCharge = 0.f;

	UPROPERTY()
	float UPCharge = 0.f;

	UPROPERTY()
	bool bFinisher = false;

	void AddToSpec(FGameplayEffectSpec& Spec) const;
};

USTRUCT(BlueprintType)
struct FAgProjectileParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Projectile", meta = (Units = "cm/s"))
	float Speed = 0.f;

	UPROPERTY(EditAnywhere, Category = "Projectile", meta = (Units = "cm"))
	float Range = 0.f;
};
