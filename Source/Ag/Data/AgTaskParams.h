// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"
#include "AgTaskParams.generated.h"

class FJsonObject;
class UAbilityTask;
class UAgGameplayAbility;
class UAnimMontage;
class UGameplayEffect;
struct FAgJsonFieldRules;
struct FAgProjectileDefinition;

enum class EAgTaskPlacement : uint8
{
	None = 0,
	Activation = 1 << 0,
	Window = 1 << 1,
	Point = 1 << 2
};
ENUM_CLASS_FLAGS(EAgTaskPlacement)

USTRUCT()
struct FAgTransition
{
	GENERATED_BODY()

	UPROPERTY()
	FName Section;

	UPROPERTY()
	float BlendTime = -1.f;

	bool IsSet() const
	{
		return !Section.IsNone();
	}
};

struct FAgTaskContext
{
	UAgGameplayAbility& Runner;

	EAgTaskPlacement Placement = EAgTaskPlacement::Activation;

	int32 MontageInstanceId = INDEX_NONE;

	float Start = 0.f;
	float End = 0.f;
};

struct FAgTaskReferences
{
	TArray<FName> Effects;

	TArray<FName> Projectiles;
};

struct FAgTaskLinks
{
	EAgTaskPlacement Placement = EAgTaskPlacement::Activation;

	const TMap<FName, TObjectPtr<UGameplayEffect>>* Effects = nullptr;

	const UAnimMontage* Montage = nullptr;

	float Start = 0.f;
	float End = 0.f;
};

/** 구조체 메타데이터는 에디터 전용이라 Task 이름 X의 Params는 이름 규칙(FAgXParams)으로 찾습니다. */
USTRUCT()
struct FAgTaskParams
{
	GENERATED_BODY()

	virtual ~FAgTaskParams() = default;

	virtual EAgTaskPlacement GetPlacements() const
	{
		return EAgTaskPlacement::None;
	}

	virtual void GetFieldRules(FAgJsonFieldRules& Rules) const
	{
	}

	virtual void Validate(const FJsonObject& Json, EAgTaskPlacement Placement, const FString& Path, TArray<FString>& Errors)
	{
	}

	virtual bool NeedsTarget() const
	{
		return false;
	}

	virtual const FAgTransition* GetTransition() const
	{
		return nullptr;
	}

	virtual bool WaitsForTransition() const
	{
		return false;
	}

	virtual bool StartsHit() const
	{
		return false;
	}

	virtual void GetReferences(FAgTaskReferences& Out) const
	{
	}

	virtual void ValidateLinks(const FAgTaskLinks& Links, const FString& Path, TArray<FString>& Errors) const
	{
	}

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const
	{
		return nullptr;
	}
};

namespace AgTaskParams
{
	const UScriptStruct* FindStruct(const FString& TaskName);

	bool Read(const FJsonObject& Item, EAgTaskPlacement Placement, FInstancedStruct& Out, const FString& Path, TArray<FString>& Errors, const TArray<FString>& Ignored = TArray<FString>());

	void ValidateReferences(const FAgTaskParams& Params, const TMap<FName, TObjectPtr<UGameplayEffect>>* Effects, const TMap<FName, FAgProjectileDefinition>* Projectiles, const FString& Path, TArray<FString>& Errors);
}
