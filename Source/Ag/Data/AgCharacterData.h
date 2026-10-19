// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/AgAbilityData.h"
#include "Math/Interval.h"
#include "AgCharacterData.generated.h"

class UAnimMontage;
class UStateTree;

USTRUCT(BlueprintType)
struct FAgAttributes
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Attributes", meta = (ClampMin = 0))
	float HP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes", meta = (ClampMin = 0))
	float MaxHP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes", meta = (ClampMin = 0))
	float SP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes", meta = (ClampMin = 0))
	float MaxSP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes", meta = (ClampMin = 0))
	float MP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes", meta = (ClampMin = 0))
	float MaxMP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes", meta = (ClampMin = 0))
	float UP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes", meta = (ClampMin = 0))
	float MaxUP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes", meta = (ClampMin = 0))
	float PP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes", meta = (ClampMin = 0))
	float MaxPP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes", meta = (ClampMin = 0))
	float ATK = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes", meta = (ClampMin = 0))
	float DEF = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes", meta = (ClampMin = 0, Units = "cm/s"))
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

	UPROPERTY(Transient, VisibleAnywhere, Category = "Abilities")
	TArray<FName> Abilities;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Death", meta = (Units = "s", ClampMin = 0))
	float DeathPresentationTime = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Attack Movement", meta = (Units = "cm", ClampMin = 0))
	float StopDistance = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Death")
	TArray<FAgDeathMotion> DeathMotions;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Abilities")
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
	UPROPERTY(Transient, VisibleAnywhere, Category = "Input", meta = (Units = "s", ClampMin = 0))
	float InputBufferTime = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Guard", meta = (ClampMin = 0))
	float GuardSPCostRatio = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Guard", meta = (ClampMin = 0))
	float GuardMoveSpeedRatio = 1.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Guard", meta = (ClampMin = 0, ClampMax = 1))
	float ParryPPRatio = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Slow Motion", meta = (ClampMin = 0.01, ClampMax = 1))
	float UltimateSlowMotionSpeed = 1.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Slow Motion", meta = (Units = "s", ClampMin = 0))
	float UltimateSlowMotionDuration = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Slow Motion", meta = (ClampMin = 0.01, ClampMax = 1))
	float PerfectDodgeSlowMotionSpeed = 1.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Slow Motion", meta = (Units = "s", ClampMin = 0))
	float PerfectDodgeSlowMotionDuration = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Lock-On", meta = (Units = "cm", ClampMin = 0))
	float LockOnReleaseRange = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Sprint", meta = (ClampMin = 0))
	float SprintSpeedRatio = 1.f;
};

UCLASS(BlueprintType)
class UAgBossData : public UAgCharacterData
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, VisibleAnywhere, Category = "AI", meta = (Units = "cm", ClampMin = 0))
	float NearDistance = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "AI", meta = (Units = "cm", ClampMin = 0))
	float FarDistance = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "AI", meta = (Units = "s", ClampMin = 0.01))
	float RetryInterval = 0.5f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "AI")
	FFloatInterval PatternRecoveryWaitPhase1 = FFloatInterval(0.f, 0.f);

	UPROPERTY(Transient, VisibleAnywhere, Category = "AI")
	FFloatInterval PatternRecoveryWaitPhase2 = FFloatInterval(0.f, 0.f);

	UPROPERTY(Transient, VisibleAnywhere, Category = "AI", meta = (Units = "s", ClampMin = 0))
	float FixedRecoveryWait = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Phase", meta = (ClampMin = 0, ClampMax = 1))
	float Phase2HPRatio = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Lock-On")
	FName LockOnBone;

	UPROPERTY(Transient, VisibleAnywhere, Category = "UI")
	FName ExecutionPromptBone;

	UPROPERTY(Transient, VisibleAnywhere, Category = "AI")
	TObjectPtr<UStateTree> AIStateTree;
};
