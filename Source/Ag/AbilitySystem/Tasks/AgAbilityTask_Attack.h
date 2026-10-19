// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgAttackData.h"
#include "Data/AgTaskParams.h"
#include "Engine/EngineBaseTypes.h"
#include "AgAbilityTask_Attack.generated.h"

class AAgCharacterBase;
class UCapsuleComponent;

USTRUCT()
struct FAgAttackParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	float DamageMultiplier = 0.f;

	UPROPERTY()
	float PoiseMultiplier = 0.f;

	UPROPERTY()
	EAgHitReaction HitReaction = EAgHitReaction::None;

	UPROPERTY()
	bool bGuardable = false;

	UPROPERTY()
	float MPCharge = 0.f;

	UPROPERTY()
	float UPCharge = 0.f;

	UPROPERTY()
	bool bFinisher = false;

	UPROPERTY()
	TOptional<float> Radius;

	UPROPERTY()
	bool bGroundOnly = false;

	FAgPlayerAttackHit MakeHit() const;

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

UCLASS()
class UAgAbilityTask_Attack : public UAbilityTask
{
	GENERATED_BODY()

public:
	UAgAbilityTask_Attack();

	static UAgAbilityTask_Attack* Attack(UGameplayAbility* Owner, const FAgAttackParams& Params, bool bOnce);
	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bOwnerFinished) override;

private:
	/** ASC가 돌리는 Task 틱과 달리 물리·포즈 갱신 뒤에 판정하도록 틱을 따로 둡니다. */
	struct FAgAttackTickFunction : public FTickFunction
	{
		UAgAbilityTask_Attack* Target = nullptr;
		virtual void ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) override;
		virtual FString DiagnosticMessage() override;
	};

	bool GetWeaponCollision(FVector& OutStart, FVector& OutEnd) const;

	void Detect();
	void SweepWeapon();
	void HitArea();

	bool Hit(AAgCharacterBase& Target, const FVector& Location);

	FAgAttackParams Params;
	TWeakObjectPtr<UCapsuleComponent> WeaponCollision;
	TSet<TWeakObjectPtr<AActor>> TouchedActors;
	FVector LastEdgeStart = FVector::ZeroVector;
	FVector LastEdgeEnd = FVector::ZeroVector;
	FAgAttackTickFunction AttackTick;
	bool bOnce = false;
};
