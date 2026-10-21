// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/AgAbilityData.h"
#include "Math/Interval.h"
#include "AgCharacterData.generated.h"

class UAnimMontage;

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

USTRUCT(BlueprintType)
struct FAgDeathMotion
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "Death")
	FName Montage;

	UPROPERTY(VisibleAnywhere, Category = "Death")
	FGameplayTag KilledBy;

	UPROPERTY(VisibleAnywhere, Category = "Death")
	FName Death;
};

UCLASS(Abstract, BlueprintType)
class UAgCharacterData : public UObject
{
	GENERATED_BODY()

public:
	bool HasValidJsonData() const
	{
		return bHasValidJsonData;
	}

	void MarkJsonDataLoaded()
	{
		bHasValidJsonData = true;
	}

	const FAgAbilityData* FindAbility(FName Id) const;

	UAnimMontage* FindMontage(FName Id) const;

	UPROPERTY(Transient, VisibleAnywhere, Category = "UI")
	FText DisplayName;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Attributes", meta = (ShowOnlyInnerProperties))
	FAgAttributes Attributes;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Abilities", meta = (DataId = "Abilities"))
	TArray<FName> Abilities;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Death")
	float DeathPresentationTime = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Attack Movement")
	float StopDistance = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Death")
	TArray<FAgDeathMotion> DeathMotions;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Abilities", meta = (DataId = "Effects"))
	TArray<FName> Effects;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Assets")
	TMap<FName, TObjectPtr<UAnimMontage>> Montages;

private:
	bool bHasValidJsonData = false;
};

UCLASS(BlueprintType)
class UAgPlayerData : public UAgCharacterData
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, VisibleAnywhere, Category = "Input")
	float InputBufferTime = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Guard")
	float GuardSPCostRatio = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Guard")
	float GuardMoveSpeedRatio = 1.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Guard", meta = (DataId = "Effects"))
	TArray<FName> ParryEffects;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Guard", meta = (DataId = "Effects"))
	TArray<FName> ParryAttackerEffects;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Dodge", meta = (DataId = "Effects"))
	TArray<FName> PerfectDodgeEffects;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Slow Motion")
	float UltimateSlowMotionSpeed = 1.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Slow Motion")
	float UltimateSlowMotionDuration = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Slow Motion")
	float PerfectDodgeSlowMotionSpeed = 1.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Slow Motion")
	float PerfectDodgeSlowMotionDuration = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Sprint")
	float SprintSpeedRatio = 1.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Movement")
	float YawRotationRate = 0.f;
};

UCLASS(BlueprintType)
class UAgBossData : public UAgCharacterData
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, VisibleAnywhere, Category = "AI")
	float NearDistance = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "AI")
	float FarDistance = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "AI")
	FFloatInterval PatternRecoveryWaitPhase1 = FFloatInterval(0.f, 0.f);

	UPROPERTY(Transient, VisibleAnywhere, Category = "AI")
	FFloatInterval PatternRecoveryWaitPhase2 = FFloatInterval(0.f, 0.f);

	UPROPERTY(Transient, VisibleAnywhere, Category = "AI")
	float FixedRecoveryWait = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Phase")
	float Phase2HPRatio = 0.f;
};
