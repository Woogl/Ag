// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "AgAbilityData.generated.h"

class FJsonObject;
class UAgCharacterData;
class UGameplayEffect;
struct FAgMontageTimeline;
struct FAgProjectileDefinition;
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

	UPROPERTY()
	FName Preset;

	UPROPERTY()
	float Range = 0.f;

	UPROPERTY()
	FGameplayTagContainer TargetRequiredTags;

	UPROPERTY()
	FGameplayTagContainer TargetBlockedTags;

	UPROPERTY()
	bool bRequired = false;

	UPROPERTY()
	float StopDistance = 0.f;

	bool bHasRange = false;
	bool bHasStopDistance = false;
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
	FAgAbilityTargeting Targeting;

	UPROPERTY()
	FAgAbilityMontage Montage;

	UPROPERTY()
	EAgAbilityEnd End = EAgAbilityEnd::Montage;

	UPROPERTY()
	TArray<FInstancedStruct> Tasks;

	UPROPERTY()
	FAgPatternSelection PatternSelection;
	bool bHasCost = false;
	bool bHasTargeting = false;
	bool bHasMontage = false;
	bool bHasPatternSelection = false;

	bool HasSameGrantSettings(const FAgAbilityData& Other) const;

	bool HasEventTrigger() const;
};

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
	bool Parse(const FJsonObject& Json, TMap<FName, FAgAbilityData>& OutAbilities, TArray<FString>& Errors);

	void ValidateCharacter(const UAgCharacterData& Data, const TMap<FName, FAgAbilityData>& Abilities, const TMap<FName, TObjectPtr<UGameplayEffect>>& Effects, const TMap<FName, FAgMontageTimeline>& Timelines, const FString& Path, TArray<FString>& Errors);

	void ValidateReferences(const TMap<FName, FAgAbilityData>& Abilities, const TMap<FName, TObjectPtr<UGameplayEffect>>* Effects, const TMap<FName, FAgProjectileDefinition>* Projectiles, TArray<FString>& Errors);
}
