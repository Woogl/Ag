// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/AgAbilityData.h"
#include "Math/Interval.h"
#include "AgCharacterData.generated.h"

class UAnimMontage;

UENUM(BlueprintType)
enum class EAgToughness : uint8
{
	Small UMETA(DisplayName = "소형"),
	Medium UMETA(DisplayName = "중형"),
	Large UMETA(DisplayName = "대형"),
};

USTRUCT(BlueprintType)
struct FAgAttributes
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Attributes")
	float HP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes")
	float MaxHP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes")
	float SP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes")
	float MaxSP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes")
	float MP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes")
	float MaxMP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes")
	float UP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes")
	float MaxUP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes")
	float PP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes")
	float MaxPP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes")
	float ATK = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes")
	float DEF = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes")
	float MOV = 0.f;
};

UCLASS(Abstract, BlueprintType)
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
};

UCLASS(BlueprintType)
class UAgPlayerData : public UAgCharacterData
{
	GENERATED_BODY()
};

UCLASS(BlueprintType)
class UAgEnemyData : public UAgCharacterData
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, VisibleAnywhere, Category = "AI")
	FFloatInterval PostPatternDelayTime = FFloatInterval(0.f, 0.f);

	UPROPERTY(Transient, VisibleAnywhere, Category = "AI")
	float SightDistance = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "AI")
	float SightAngle = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Phase")
	TArray<float> PhaseTransitionHPRatios;
};
