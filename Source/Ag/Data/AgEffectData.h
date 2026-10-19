// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "AgEffectData.generated.h"

class UAgHitResultGameplayEffectComponent;
class UGameplayEffectExecutionCalculation;

UENUM()
enum class EAgEffect : uint8
{
	Damage,
	Resource,
	Cooldown,
	Hitstop,
	SPRegenDelay,
	SPRegenDelayEmpty,
	PPRegenDelay,
	Kill,
	PPRefill,
	ParryPPLoss,
	ParryReward,
	PerfectDodgeReward,
	DodgeCounterChance,
};

UENUM()
enum class EAgModifierOp : uint8
{
	Add,
	Override
};

USTRUCT()
struct FAgEffectModifier
{
	GENERATED_BODY()

	UPROPERTY()
	TFieldPath<FProperty> Attribute;

	UPROPERTY()
	EAgModifierOp Op = EAgModifierOp::Add;

	UPROPERTY()
	TOptional<float> Value;

	UPROPERTY()
	TOptional<float> PerSecond;

	UPROPERTY()
	FGameplayTag SetByCaller;

	UPROPERTY()
	TFieldPath<FProperty> FromAttribute;
};

USTRUCT()
struct FAgEffectDefinition
{
	GENERATED_BODY()

	UPROPERTY()
	EGameplayEffectDurationType DurationPolicy = EGameplayEffectDurationType::Instant;

	UPROPERTY()
	TOptional<float> Duration;

	UPROPERTY()
	TOptional<float> Period;

	UPROPERTY()
	TOptional<bool> ExecutePeriodicOnApplication;

	UPROPERTY()
	TOptional<EGameplayEffectPeriodInhibitionRemovedPolicy> PeriodicInhibitionPolicy;

	UPROPERTY()
	TArray<FAgEffectModifier> Modifiers;

	UPROPERTY()
	TArray<TSubclassOf<UGameplayEffectExecutionCalculation>> Executions;

	UPROPERTY()
	FGameplayTagContainer AssetTags;

	UPROPERTY()
	FGameplayTagContainer GrantedTags;

	UPROPERTY()
	FGameplayTagContainer ApplicationIgnoredTags;

	UPROPERTY()
	FGameplayTagContainer OngoingRequiredTags;

	UPROPERTY()
	FGameplayTagContainer OngoingIgnoredTags;

	UPROPERTY()
	TArray<TSubclassOf<UAgHitResultGameplayEffectComponent>> Components;
};

UCLASS()
class UAgEffectSet : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, VisibleAnywhere, Category = "Effects")
	TMap<FName, FAgEffectDefinition> Definitions;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Effects")
	TMap<FName, TObjectPtr<UGameplayEffect>> Effects;
};

namespace AgEffectData
{
	FName Id(EAgEffect Effect);

	/**
	 * 객체 이름에 Version을 붙여 읽을 때마다 새 객체를 만듭니다. 같은 이름으로 다시 만들면 엔진이 기존 객체를
	 * 그 자리에서 부수고 다시 만들어, 걸려 있는 효과가 망가지기 때문입니다.
	 */
	void Build(const TMap<FName, FAgEffectDefinition>& Definitions, UObject& Outer, int32 Version, TMap<FName, TObjectPtr<UGameplayEffect>>& Out);
}
