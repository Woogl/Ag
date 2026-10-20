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

	/**
	 * 객체 이름에 Version을 붙여 읽을 때마다 새 객체를 만듭니다. 같은 이름으로 다시 만들면 엔진이 기존 객체를
	 * 그 자리에서 부수고 다시 만들어, 걸려 있는 효과가 망가지기 때문입니다.
	 */
	UGameplayEffect* MakeEffect(FName Id, const FAgEffectDefinition& Definition, UObject& Outer, int32 Version);
}
