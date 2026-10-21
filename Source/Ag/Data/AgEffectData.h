// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "AgEffectData.generated.h"

UENUM()
enum class EAgEffect : uint8
{
	Damage,
	Resource,
	Cooldown,
	Kill,
	Hitstop,
	PPRefill,
};

USTRUCT()
struct FAgEffectModifier
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayAttribute Attribute;

	UPROPERTY()
	TOptional<float> Value;

	UPROPERTY()
	TOptional<float> PerSecond;

	UPROPERTY()
	TOptional<float> Multiplier;

	UPROPERTY()
	FGameplayAttribute FromAttribute;

	UPROPERTY()
	float Coefficient = 1.f;

	UPROPERTY()
	FGameplayTagContainer RequiredTags;

	UPROPERTY()
	FGameplayTagContainer IgnoredTags;
};

USTRUCT()
struct FAgRegenDelay
{
	GENERATED_BODY()

	UPROPERTY(meta = (DataId = "Effects"))
	FName Effect;

	UPROPERTY(meta = (DataId = "Effects"))
	FName EffectAtZero;
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
	TArray<FAgEffectModifier> Modifiers;

	UPROPERTY()
	FGameplayTagContainer GrantedTags;

	UPROPERTY()
	FGameplayTagContainer OngoingIgnoredTags;

	UPROPERTY()
	TOptional<FAgRegenDelay> RegenDelay;

	UPROPERTY()
	float SlowMotionSpeed = 1.f;

	UPROPERTY()
	float SlowMotionDuration = 0.f;
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
	UGameplayEffect* MakeNativeEffect(EAgEffect Id, UObject& Outer);

	UGameplayEffect* MakeEffect(const FAgEffectDefinition& Definition, UObject& Outer);
}
