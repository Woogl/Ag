// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AgHitReward.generated.h"

struct FGameplayEffectSpec;

USTRUCT()
struct FAgHitReward
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag AbilityTag;

	/** 비어 있으면 섹션을 가리지 않으며, 적중한 순간 공격자가 재생 중인 몽타주 섹션과 맞춥니다. */
	UPROPERTY()
	FName Section;

	UPROPERTY()
	float MP = 0.f;

	UPROPERTY()
	float UP = 0.f;

	void AddToSpec(FGameplayEffectSpec& Spec) const;
};
