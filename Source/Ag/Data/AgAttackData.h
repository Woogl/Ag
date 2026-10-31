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
	bool bFinisher = false;

	UPROPERTY()
	bool bAttackerHitstop = true;

	void AddToSpec(FGameplayEffectSpec& Spec) const;
};
