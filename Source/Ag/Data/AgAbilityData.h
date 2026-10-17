// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/AgAbilityTaskParams.h"
#include "AgAbilityData.generated.h"

class FJsonObject;
class UAgCharacterData;

USTRUCT()
struct FAgAbilityActivation
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTagContainer RequiredTags;

	UPROPERTY()
	FGameplayTagContainer BlockedTags;

	UPROPERTY()
	FGameplayTagContainer OwnedTags;

	UPROPERTY()
	FGameplayTagContainer BlockTags;

	UPROPERTY()
	FGameplayTagContainer CancelTags;

	UPROPERTY()
	FGameplayTagContainer TriggerEvents;

	UPROPERTY()
	bool bAllowRetrigger = false;
};

USTRUCT()
struct FAgPatternSelection
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<int32> Phases;

	UPROPERTY()
	float WeightNear = 0.f;

	UPROPERTY()
	float WeightMid = 0.f;

	UPROPERTY()
	float WeightFar = 0.f;

	UPROPERTY()
	float ResponseRange = 0.f;

	UPROPERTY()
	float ResponseChance = 0.f;
};

/** 행동별 스펙 정의. 실행 중에는 Ability가 이 값과 에셋 참조를 복사하여 보유합니다. */
USTRUCT()
struct FAgAbilityData
{
	GENERATED_BODY()

	UPROPERTY()
	FName Id;

	UPROPERTY()
	FGameplayTagContainer Tags;

	UPROPERTY()
	float Cooldown = 0.f;

	UPROPERTY()
	FAgAbilityActivation Activation;

	UPROPERTY()
	FAgResourceCostParams Cost;

	UPROPERTY()
	FAgAbilityTasks Tasks;

	UPROPERTY()
	FAgPatternSelection PatternSelection;
	bool bHasCost = false;
	bool bHasPatternSelection = false;

	bool HasSameGrantSettings(const FAgAbilityData& Other) const;
};

namespace AgAbilityData
{
	/** 고정 스키마만 읽습니다. 실패 시 OutAbilities를 변경하지 않습니다. */
	bool Parse(const FJsonObject& Json, TArray<FAgAbilityData>& OutAbilities, TArray<FString>& Errors);
	void ValidateReferences(const UAgCharacterData& Data, TArray<FString>& Errors);
	TArray<FName> MontageIds(const UAgCharacterData& Data, const FAgAbilityData& Ability);
}
