// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/AgJsonData.h"
#include "AgCombatRules.generated.h"

UCLASS()
class UAgCombatRules : public UAgJsonData
{
	GENERATED_BODY()

public:
	static const UAgCombatRules& Get()
	{
		return UAgAssetManager::Get().GetData<UAgCombatRules>();
	}

	virtual const TCHAR* GetFileName() const override
	{
		return TEXT("CombatRules.json");
	}

	virtual bool IsAlwaysLoaded() const override
	{
		return true;
	}

	UPROPERTY(Transient, VisibleAnywhere, Category = "Hit")
	float GuardReduction = 1.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Hit")
	float DefenseConstant = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Hit")
	float HitstopDuration = 0.f;

	/** 극한 회피 판정 상자의 한 변 길이(cm)입니다. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Hit")
	float PerfectDodgeBoxSize = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Bonus Damage")
	float GroggyBonusDamage = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Bonus Damage")
	float BackAttackBonusDamage = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Bonus Damage")
	float AerialBonusDamage = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Fall")
	float FallDamageStartHeight = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Fall")
	float FallDeathHeight = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "AI")
	float TargetLostTime = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, Category = "AI")
	int32 MaxAttackers = 0;

	UPROPERTY(Transient, VisibleAnywhere, Category = "AI")
	float ReattackWaitTime = 0.f;
};
