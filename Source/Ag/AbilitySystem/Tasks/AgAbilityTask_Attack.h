// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgAbilityTaskParams.h"
#include "Engine/EngineBaseTypes.h"
#include "AgAbilityTask_Attack.generated.h"

class AAgCharacterBase;
class UCapsuleComponent;

/** 한 판정의 적중과 수명만 소유합니다. 이동·모션·다른 Task를 제어하지 않습니다. */
UCLASS()
class UAgAbilityTask_Attack : public UAbilityTask
{
	GENERATED_BODY()

#if WITH_DEV_AUTOMATION_TESTS
	friend class FAgAbilityExecutionTest;
#endif

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
