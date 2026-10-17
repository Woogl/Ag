// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/AgTaskParams.h"
#include "GameplayTagContainer.h"
#include "AgAbilityTaskParams.generated.h"

struct FGameplayEffectSpec;

UENUM()
enum class EAgResource : uint8
{
	SP,
	MP,
	UP
};

USTRUCT()
struct FAgResourceCostParams
{
	GENERATED_BODY()

	UPROPERTY()
	EAgResource Resource = EAgResource::SP;

	UPROPERTY()
	float Amount = 0.f;

	/** 자원 효과(Resource) 스펙에 이 비용을 음수로 넣습니다. 다른 자원은 0입니다. */
	void AddToSpec(FGameplayEffectSpec& Spec) const;
};

/*
 * 아래 Params는 행동 처리가 발동 Task 배열과 시간표에서 값을 직접 읽는 설정입니다. 범용 Task와 효과 데이터로 옮기면
 * 없어집니다(구현 계획서 '단일 Ability Runner와 독립 기능 Task 설계안'의 '구현 순서와 완료 조건' ②단계).
 */

USTRUCT()
struct FAgDrainResourceParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	EAgResource Resource = EAgResource::SP;

	UPROPERTY()
	float AmountPerSecond = 0.f;

	/** 소모 효과(ResourceDrain) 스펙에 한 주기에 줄일 양(초당 소모량 × 스펙의 주기)을 음수로 넣습니다. 다른 자원은 0입니다. */
	void AddToSpec(FGameplayEffectSpec& Spec) const;

	virtual EAgTaskPlacement GetPlacements() const override
	{
		return EAgTaskPlacement::Activation;
	}

	virtual void Validate(const FJsonObject& Json, const FString& Path, TArray<FString>& Errors) override;
};

USTRUCT()
struct FAgTimedTagParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag Tag;

	UPROPERTY()
	float Duration = 0.f;

	virtual EAgTaskPlacement GetPlacements() const override
	{
		return EAgTaskPlacement::Activation;
	}

	virtual void Validate(const FJsonObject& Json, const FString& Path, TArray<FString>& Errors) override;
};

USTRUCT()
struct FAgGrantResourcesParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	float SP = 0.f;

	UPROPERTY()
	float MP = 0.f;

	UPROPERTY()
	float UP = 0.f;

	/** 자원 효과(Resource) 스펙에 얻는 SP·MP·UP를 넣습니다. */
	void AddToSpec(FGameplayEffectSpec& Spec) const;

	virtual EAgTaskPlacement GetPlacements() const override
	{
		return EAgTaskPlacement::Activation;
	}

	virtual void Validate(const FJsonObject& Json, const FString& Path, TArray<FString>& Errors) override;
};

USTRUCT()
struct FAgSlowMotionParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	float Speed = 0.f;

	UPROPERTY()
	float RealDuration = 0.f;

	virtual EAgTaskPlacement GetPlacements() const override
	{
		return EAgTaskPlacement::Activation;
	}

	virtual void Validate(const FJsonObject& Json, const FString& Path, TArray<FString>& Errors) override;
};

USTRUCT()
struct FAgLaunchVerticalParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	float VelocityZ = 0.f;

	virtual EAgTaskPlacement GetPlacements() const override
	{
		return EAgTaskPlacement::Activation;
	}

	virtual void GetFieldRules(FAgJsonFieldRules& Rules) const override;
};

USTRUCT()
struct FAgLaunchToTargetParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	float AirTime = 0.f;

	UPROPERTY()
	float TargetOffset = 0.f;

	UPROPERTY()
	float MaxDistance = 0.f;

	virtual EAgTaskPlacement GetPlacements() const override
	{
		return EAgTaskPlacement::Activation;
	}

	virtual void Validate(const FJsonObject& Json, const FString& Path, TArray<FString>& Errors) override;
};

USTRUCT()
struct FAgTargetQueryParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	float Range = 0.f;

	virtual EAgTaskPlacement GetPlacements() const override
	{
		return EAgTaskPlacement::Activation;
	}
};

/** 재생을 소유한 행동의 사건 처리로 보내는 사건(시간표 지점) */
USTRUCT()
struct FAgGameplayEventParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag Event;

	virtual EAgTaskPlacement GetPlacements() const override
	{
		return EAgTaskPlacement::Point;
	}

	virtual void Validate(const FJsonObject& Json, const FString& Path, TArray<FString>& Errors) override;
};
