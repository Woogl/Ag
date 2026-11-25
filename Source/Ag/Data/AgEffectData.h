// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/AgJsonData.h"
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
	FallDamage,
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

	/** 값이 있으면 대상의 BackingAttribute에 이 값을 곱해 더합니다. */
	UPROPERTY()
	TOptional<float> Coefficient;

	UPROPERTY()
	FGameplayAttribute BackingAttribute;
};

USTRUCT()
struct FAgRegenDelay
{
	GENERATED_BODY()

	UPROPERTY(meta = (DataId = "Effects"))
	FName Effect;
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
	FGameplayTagContainer RemovalTags;

	UPROPERTY()
	TOptional<FAgRegenDelay> RegenDelay;

	UPROPERTY()
	float SlowMotionSpeed = 1.f;

	UPROPERTY()
	float SlowMotionDuration = 0.f;
};

UCLASS()
class UAgEffectSet : public UAgJsonData
{
	GENERATED_BODY()

public:
	/** `Effects.json`의 ID로 찾습니다. */
	static const UGameplayEffect* FindEffect(FName Id);

	/** 코드가 고르는 효과입니다. */
	static const UGameplayEffect* GetEffect(EAgEffect Effect);

	virtual const TCHAR* GetFileName() const override
	{
		return TEXT("Effects.json");
	}

	virtual bool IsAlwaysLoaded() const override
	{
		return true;
	}

	virtual bool ReadJson(FAgJsonReader& Reader, FName Key) override;

	virtual void CopyJson(const UAgJsonData& Loaded) override;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Effects")
	TMap<FName, FAgEffectDefinition> Definitions;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Effects")
	TMap<FName, TObjectPtr<UGameplayEffect>> Effects;

private:
	// JSON 필드(Transient)로 표시하지 않아야 다시 읽을 때 값 복사가 비우지 않습니다.
	UPROPERTY()
	TMap<EAgEffect, TObjectPtr<UGameplayEffect>> NativeEffects;
};

namespace AgEffectData
{
	UGameplayEffect* MakeNativeEffect(EAgEffect Id, UObject& Outer);

	UGameplayEffect* MakeEffect(const FAgEffectDefinition& Definition, UObject& Outer);

	/** 넣지 않은 SetByCaller 값은 엔진이 오류로 남기므로, Resource 효과에는 바꾸지 않는 자원도 0을 넣습니다. */
	void SetResourceAmounts(FGameplayEffectSpec& Spec, float MP, float UP);
}
