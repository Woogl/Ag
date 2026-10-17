// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/AgAbilityTaskParams.h"
#include "AgAbilityData.generated.h"

class FJsonObject;
class UAgCharacterData;
class UGameplayEffect;

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

/** Abilities.json의 값: 어빌리티 ID마다 정의(구현 계획서 'JSON 데이터'). 캐릭터는 어빌리티 목록에 ID를 적어 씁니다. */
UCLASS()
class UAgAbilitySet : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, VisibleAnywhere, Category = "Abilities")
	TMap<FName, FAgAbilityData> Abilities;
};

namespace AgAbilityData
{
	/** 고정 스키마만 읽고, C++ 사건이 요구하는 이름을 검사합니다. 실패 시 OutAbilities를 변경하지 않습니다. */
	bool Parse(const FJsonObject& Json, TMap<FName, FAgAbilityData>& OutAbilities, TArray<FString>& Errors);

	/** 캐릭터의 어빌리티·효과 ID와, 그 어빌리티들이 이 캐릭터에서 쓰는 몽타주를 검사합니다. 오류에는 Path를 앞에 붙입니다. */
	void ValidateCharacter(const UAgCharacterData& Data, const TMap<FName, FAgAbilityData>& Abilities,
	    const TMap<FName, TObjectPtr<UGameplayEffect>>& Effects, const FString& Path, TArray<FString>& Errors);

	TArray<FName> MontageIds(const UAgCharacterData& Data, const FAgAbilityData& Ability);
}
