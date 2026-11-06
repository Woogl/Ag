// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/AgAbilityData.h"
#include "UObject/Object.h"
#include "AgCombatRules.generated.h"

UCLASS()
class UAgCombatRules : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, VisibleAnywhere, Category = "Hit")
	float GuardReduction = 1.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Hit")
	float DefenseConstant = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Hit")
	float HitstopDuration = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Guard")
	float GuardSPCostRatio = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Parry")
	float ParryHPMultiplier = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Parry")
	float ParryPPMultiplier = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Fall")
	float FallDamageStartHeight = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Fall")
	float FallDeathHeight = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "AI")
	float TargetLostTime = 0.f;

	/** 락온 대상이 없는 캐릭터가 쳐다볼 상대를 찾는 조건입니다. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "LookAt")
	FAgTargetingRequest LookAtTargeting;
};
