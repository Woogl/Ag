// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/AgAbilityData.h"
#include "Math/Interval.h"
#include "AgCharacterData.generated.h"

class UAnimMontage;

UENUM()
enum class EAgToughness : uint8
{
	Small,
	Medium,
	Large,
};

USTRUCT()
struct FAgAttributes
{
	GENERATED_BODY()

	UPROPERTY()
	float HP = 0.f;

	UPROPERTY()
	float MaxHP = 0.f;

	UPROPERTY()
	float SP = 0.f;

	UPROPERTY()
	float MaxSP = 0.f;

	UPROPERTY()
	float MP = 0.f;

	UPROPERTY()
	float MaxMP = 0.f;

	UPROPERTY()
	float UP = 0.f;

	UPROPERTY()
	float MaxUP = 0.f;

	UPROPERTY()
	float PP = 0.f;

	UPROPERTY()
	float MaxPP = 0.f;

	UPROPERTY()
	float ATK = 0.f;

	UPROPERTY()
	float DEF = 0.f;

	UPROPERTY()
	float MOV = 0.f;
};

UCLASS()
class UAgCharacterData : public UObject
{
	GENERATED_BODY()

public:
	const FAgAbilityData* FindAbility(FName Id) const;

	UAnimMontage* FindMontage(FName Id) const;

	UPROPERTY(Transient, VisibleAnywhere, Category = "UI")
	FText DisplayName;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Combat")
	EAgToughness Toughness = EAgToughness::Medium;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Attributes", meta = (ShowOnlyInnerProperties))
	FAgAttributes Attributes;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Abilities", meta = (DataId = "Abilities"))
	TArray<FName> Abilities;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Abilities", meta = (DataId = "Effects"))
	TArray<FName> Effects;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Assets")
	TMap<FName, TObjectPtr<UAnimMontage>> Montages;

	UPROPERTY(Transient, VisibleAnywhere, Category = "AI")
	FFloatInterval PostPatternDelayTime = FFloatInterval(0.f, 0.f);

	UPROPERTY(Transient, VisibleAnywhere, Category = "AI")
	float SightDistance = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "AI")
	float SightAngle = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Phase")
	TArray<float> PhaseTransitionHPRatios;
};

UCLASS()
class UAgCharacterSet : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, VisibleAnywhere, Category = "Characters")
	TMap<FName, TObjectPtr<UAgCharacterData>> Characters;
};
