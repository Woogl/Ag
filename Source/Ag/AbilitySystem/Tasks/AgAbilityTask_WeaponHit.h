// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgAttackData.h"
#include "Data/AgTaskParams.h"
#include "Engine/EngineBaseTypes.h"
#include "AgAbilityTask_WeaponHit.generated.h"

class AAgCharacterBase;
class UAgAbilityTask_PlayMontage;
class UCapsuleComponent;
struct FHitResult;
struct FAgHitState;

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
	friend struct FAgWeaponHitParams;

public:
	UAgAbilityTask_WeaponHit();

	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	/**
	 * 엔진 Task 틱(ASC 틱) 대신 틱을 따로 둡니다. Task 틱으로 스윕하면 이번 프레임에 열린 구간의 첫 피해가 다음 프레임으로
	 * 밀리고, 같은 프레임에 끝나는 상대의 무적·극한 회피 구간이 반영될지가 캐릭터의 틱 순서에 달립니다.
	 */
	struct FAgSweepTickFunction : public FTickFunction
	{
		UAgAbilityTask_WeaponHit* Target = nullptr;
		virtual void ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) override;
		virtual FString DiagnosticMessage() override;
	};

	struct FAgWeaponEdge
	{
		TWeakObjectPtr<UCapsuleComponent> Capsule;
		FName HandBone;
		FVector LastStart = FVector::ZeroVector;
		FVector LastEnd = FVector::ZeroVector;
	};

	void SweepEdges(float AlphaEnd = 1.f);
	bool Sweep(FAgWeaponEdge& Edge, float AlphaEnd);
	bool Hit(AAgCharacterBase& Target, const FVector& Location, const FHitResult& Contact);
	bool GetPreviousEdge(const FAgWeaponEdge& Edge, FVector& OutStart, FVector& OutEnd) const;

	UPROPERTY()
	FAgWeaponHitParams Params;

	TOptional<FFloatInterval> Window;
	TWeakObjectPtr<UAgAbilityTask_PlayMontage> Playback;
	TSharedPtr<FAgHitState> HitState;
	TArray<FAgWeaponEdge> Edges;
	TSet<TWeakObjectPtr<AActor>> TouchedActors;
	FAgSweepTickFunction SweepTick;
	float LastPosition = 0.f;
	bool bSweeping = false;
};
