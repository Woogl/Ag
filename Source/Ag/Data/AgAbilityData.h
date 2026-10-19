// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "AgAbilityData.generated.h"

struct FGameplayEffectSpec;

UENUM()
enum class EAgResource : uint8
{
	SP,
	MP,
	UP
};

namespace AgResource
{
	/** 넣지 않은 SetByCaller 값은 엔진이 오류로 남기므로 바꾸지 않는 자원도 0을 넣습니다. */
	void SetAmounts(FGameplayEffectSpec& Spec, float SP, float MP, float UP);

	void SetAmount(FGameplayEffectSpec& Spec, EAgResource Resource, float Amount);
}

USTRUCT()
struct FAgResourceCostParams
{
	GENERATED_BODY()

	UPROPERTY()
	EAgResource Resource = EAgResource::SP;

	UPROPERTY()
	float Amount = 0.f;

	void AddToSpec(FGameplayEffectSpec& Spec) const;
};

UENUM()
enum class EAgTriggerSource : uint8
{
	GameplayEvent,
	OwnedTagPresent
};

USTRUCT()
struct FAgAbilityTrigger
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag Tag;

	UPROPERTY()
	EAgTriggerSource Source = EAgTriggerSource::GameplayEvent;
};

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
	TArray<FAgAbilityTrigger> Triggers;

	UPROPERTY()
	bool bAllowRetrigger = false;
};

USTRUCT()
struct FAgAbilityTargeting
{
	GENERATED_BODY()

	UPROPERTY(meta = (DataId = "TargetingPresets"))
	FName Preset;

	UPROPERTY()
	TOptional<float> Range;

	UPROPERTY()
	FGameplayTagContainer TargetRequiredTags;

	UPROPERTY()
	FGameplayTagContainer TargetBlockedTags;

	UPROPERTY()
	bool bRequired = false;

	UPROPERTY()
	TOptional<float> StopDistance;
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

USTRUCT()
struct FAgAbilityMontage
{
	GENERATED_BODY()

	UPROPERTY()
	FName MontageId;
};

UENUM()
enum class EAgAbilityEnd : uint8
{
	Montage,
	TriggerTag
};

USTRUCT()
struct FAgAbilityData
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTagContainer Tags;

	UPROPERTY()
	float Cooldown = 0.f;

	UPROPERTY()
	FAgAbilityActivation Activation;

	UPROPERTY()
	TOptional<FAgResourceCostParams> Cost;

	UPROPERTY()
	TOptional<FAgAbilityTargeting> Targeting;

	UPROPERTY()
	TOptional<FAgAbilityMontage> Montage;

	UPROPERTY()
	EAgAbilityEnd End = EAgAbilityEnd::Montage;

	UPROPERTY()
	TArray<FInstancedStruct> Tasks;

	UPROPERTY()
	TOptional<FAgPatternSelection> PatternSelection;

	bool HasSameGrantSettings(const FAgAbilityData& Other) const;
};

UCLASS()
class UAgAbilitySet : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, VisibleAnywhere, Category = "Abilities")
	TMap<FName, FAgAbilityData> Abilities;
};
