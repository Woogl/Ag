// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgAttackData.h"
#include "Data/AgTaskParams.h"
#include "Engine/EngineBaseTypes.h"
#include "AgAbilityTask_WeaponHit.generated.h"

class AAgCharacterBase;
class UCapsuleComponent;

USTRUCT()
struct FAgWeaponHitParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	FAgDamage Damage;

	UPROPERTY()
	FName ComponentTag = TEXT("WeaponCollision");

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

UCLASS()
class UAgAbilityTask_WeaponHit : public UAbilityTask
{
	GENERATED_BODY()

public:
	UAgAbilityTask_WeaponHit();

	static UAgAbilityTask_WeaponHit* WeaponHit(UGameplayAbility* Owner, const FAgWeaponHitParams& Params);
	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	/** ASC가 돌리는 Task 틱과 달리 물리·포즈 갱신 뒤에 판정하도록 틱을 따로 둡니다. */
	struct FAgSweepTickFunction : public FTickFunction
	{
		UAgAbilityTask_WeaponHit* Target = nullptr;
		virtual void ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) override;
		virtual FString DiagnosticMessage() override;
	};

	struct FAgWeaponEdge
	{
		TWeakObjectPtr<UCapsuleComponent> Capsule;
		FVector LastStart = FVector::ZeroVector;
		FVector LastEnd = FVector::ZeroVector;
	};

	static bool GetEdge(const UCapsuleComponent* Capsule, FVector& OutStart, FVector& OutEnd);

	void SweepEdges();
	bool Sweep(FAgWeaponEdge& Edge);
	bool Hit(AAgCharacterBase& Target, const FVector& Location);

	FAgWeaponHitParams Params;
	TArray<FAgWeaponEdge> Edges;
	TSet<TWeakObjectPtr<AActor>> TouchedActors;
	FAgSweepTickFunction SweepTick;
};
