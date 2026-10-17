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

/**
 * 한 타의 판정 데이터(Attack Task의 Params). 시간표 구간이면 그동안 무기 판정을 하고, 발동 Task 배열의 항목은 행동 처리가
 * 정한 대상을 차례대로 한 번씩 칩니다.
 */
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

	FAgPlayerAttackHit MakeHit(bool bFinisher = false) const;

	virtual EAgTaskPlacement GetPlacements() const override
	{
		return EAgTaskPlacement::Activation | EAgTaskPlacement::Window;
	}

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

/** 한 판정의 적중과 수명만 소유합니다. 이동·모션·다른 Task를 제어하지 않습니다. */
UCLASS()
class UAgAbilityTask_Attack : public UAbilityTask
{
	GENERATED_BODY()

public:
	UAgAbilityTask_Attack();

	static UAgAbilityTask_Attack* Sweep(UGameplayAbility* Owner, const FAgAttackParams& Params);
	static UAgAbilityTask_Attack* Strike(UGameplayAbility* Owner, const FAgAttackParams& Params, const TArray<AAgCharacterBase*>& Targets,
	    bool bFinisher, TOptional<FVector> HitLocation = {});
	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bOwnerFinished) override;

private:
	/** ASC의 다른 Task와 독립적으로 물리·포즈 갱신 뒤 판정합니다. */
	struct FAgAttackTickFunction : public FTickFunction
	{
		UAgAbilityTask_Attack* Target = nullptr;
		virtual void ExecuteTick(
		    float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) override;
		virtual FString DiagnosticMessage() override;
	};

	bool GetWeaponCollision(FVector& OutStart, FVector& OutEnd) const;
	void SweepWeapon();

	FAgAttackParams Params;
	TOptional<FVector> HitLocation;
	TArray<TWeakObjectPtr<AAgCharacterBase>> Targets;
	TWeakObjectPtr<UCapsuleComponent> WeaponCollision;
	TSet<TWeakObjectPtr<AActor>> TouchedActors;
	FVector LastEdgeStart = FVector::ZeroVector;
	FVector LastEdgeEnd = FVector::ZeroVector;
	FAgAttackTickFunction AttackTick;
	bool bSingleStrike = false;
	bool bFinisher = false;
};
