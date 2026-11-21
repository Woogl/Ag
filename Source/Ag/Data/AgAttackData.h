// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AgAttackData.generated.h"

struct FGameplayEffectSpec;

UENUM()
enum class EAgHitReaction : uint8
{
	None,
	Flinch,
	KnockDown,
	KnockUp,
};

UENUM()
enum class EAgAttackType : uint8
{
	Normal,
	Unblockable,
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
	bool bIgnoreToughness = false;

	UPROPERTY()
	EAgAttackType AttackType = EAgAttackType::Normal;

	UPROPERTY()
	bool bFinisher = false;

	void AddToSpec(FGameplayEffectSpec& Spec) const;
};
