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
 * 한 타의 판정 데이터(Attack Task의 Params). 구간이면 켜진 동안 판정 범위에 닿는 적을 대상마다 한 번 치고, 지점이면 그 시각에
 * 닿아 있는 적을 칩니다. 판정 범위는 무기(무기 충돌 스윕)이고, Radius가 있으면 주인 둘레의 공격 범위입니다.
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

	/** 선택. 마지막 일격(Effect.Damage.Finisher)입니다. */
	UPROPERTY()
	bool bFinisher = false;

	/** 선택. 주인 둘레의 공격 범위(캡슐 중심 사이 수평 거리, cm). 없으면 무기 판정입니다. */
	UPROPERTY()
	float Radius = 0.f;

	/** 선택. 공중의 대상(State.Airborne)은 맞지 않습니다. */
	UPROPERTY()
	bool bGroundOnly = false;

	bool bHasRadius = false;

	FAgPlayerAttackHit MakeHit() const;

	virtual EAgTaskPlacement GetPlacements() const override
	{
		return EAgTaskPlacement::Window | EAgTaskPlacement::Point;
	}

	virtual void GetFieldRules(FAgJsonFieldRules& Rules) const override;
	virtual void Validate(const FJsonObject& Json, EAgTaskPlacement Placement, const FString& Path, TArray<FString>& Errors) override;
	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

/** 한 판정의 적중과 수명만 소유합니다. 이동·모션·다른 Task를 제어하지 않습니다. */
UCLASS()
class UAgAbilityTask_Attack : public UAbilityTask
{
	GENERATED_BODY()

public:
	UAgAbilityTask_Attack();

	/** bOnce면 시작할 때 한 번 판정하고 끝납니다(지점). */
	static UAgAbilityTask_Attack* Attack(UGameplayAbility* Owner, const FAgAttackParams& Params, bool bOnce);
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

	/** 판정 범위에 닿는 적을 칩니다. 이 판정에서 이미 친 대상은 다시 치지 않습니다. */
	void Detect();
	void SweepWeapon();
	void HitArea();

	/** 치면 거짓을 돌려줍니다: 판정이나 행동이 이 타격으로 끝났습니다. */
	bool Hit(AAgCharacterBase& Target, const FVector& Location);

	FAgAttackParams Params;
	TWeakObjectPtr<UCapsuleComponent> WeaponCollision;
	TSet<TWeakObjectPtr<AActor>> TouchedActors;
	FVector LastEdgeStart = FVector::ZeroVector;
	FVector LastEdgeEnd = FVector::ZeroVector;
	FAgAttackTickFunction AttackTick;
	bool bOnce = false;
};
