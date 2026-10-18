// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_Dash.generated.h"

struct FGameplayEventData;

/** Dash가 움직이는 쪽 */
UENUM()
enum class EAgDashTo : uint8
{
	/** 행동 대상 쪽으로 수평 돌진 */
	Target,

	/** 수평 속도는 그대로 두고 아래로 */
	Down
};

/**
 * 돌진·내리꽂기(Dash Task의 Params). Speed(cm/s)의 일정 속도로 움직이고, 스스로 멈추면 Next로 전환합니다. To Target만
 * HomingTurnRate(초당 최대 수평 회전 각도), Overshoot, MaxDistance를 씁니다.
 */
USTRUCT()
struct FAgDashParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	float Speed = 0.f;

	UPROPERTY()
	EAgDashTo To = EAgDashTo::Target;

	/** 선택 */
	UPROPERTY()
	FAgTransition Next;

	/** 선택. 없으면 곧게 갑니다. */
	UPROPERTY()
	float HomingTurnRate = 0.f;

	/** 선택. 출발할 때 대상까지의 수평 거리에 더하는 거리(cm) */
	UPROPERTY()
	float Overshoot = 0.f;

	/** 선택. 가는 거리(cm)의 상한. 대상이 없으면 이 거리를 곧게 갑니다. */
	UPROPERTY()
	float MaxDistance = 0.f;

	bool bHasMaxDistance = false;

	virtual EAgTaskPlacement GetPlacements() const override
	{
		return EAgTaskPlacement::Window;
	}

	virtual void GetFieldRules(FAgJsonFieldRules& Rules) const override;
	virtual void Validate(const FJsonObject& Json, EAgTaskPlacement Placement, const FString& Path, TArray<FString>& Errors) override;

	virtual bool NeedsTarget() const override
	{
		return To == EAgDashTo::Target;
	}

	virtual const FAgTransition* GetTransition() const override
	{
		return Next.IsSet() ? &Next : nullptr;
	}

	virtual bool WaitsForTransition() const override
	{
		return Next.IsSet();
	}

	/** 루트 모션 소스로 움직이는 구간에는 몽타주 루트 모션이 없습니다. */
	virtual void ValidateLinks(const FAgTaskLinks& Links, const FString& Path, TArray<FString>& Errors) const override;
	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

/**
 * 엔진 일정 힘 루트 모션 Task를 끝없는 지속 시간으로 걸고, 끝나면 속도를 0으로 둡니다. 충돌은 그대로 적용됩니다. To Target은 주인이
 * 바라보는 쪽으로 출발해 행동 대상 쪽으로 방향을 바꾸며, 정한 거리를 실제 경로로 가거나 벽에 막히거나 공격이 막히면
 * (Event.AttackBlocked) 그 자리에서 멈춥니다. To Down은 공중이 아니게 되면 멈춥니다. 스스로 멈추면 Next로 전환하고, 구간이 끝나면
 * 전환하지 않고 멈춥니다. 소스를 걸고 지우는 것은 엔진 Task가 합니다.
 */
UCLASS()
class UAgAbilityTask_Dash : public UAbilityTask_ApplyRootMotionConstantForce
{
	GENERATED_BODY()

public:
	static UAgAbilityTask_Dash* Dash(UGameplayAbility* Owner, const FAgDashParams& Params, int32 MontageInstanceId);

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	void TickTarget(float DeltaTime);
	void HandleAttackBlocked(const FGameplayEventData* Payload);

	/** 그 자리에서 멈추고 Next로 전환합니다. */
	void Stop();

	FAgDashParams Params;
	int32 MontageInstanceId = INDEX_NONE;

	/** To Target이 가는 거리(cm) */
	float Distance = 0.f;
	float TraveledDistance = 0.f;
	TWeakObjectPtr<AActor> Target;
	FVector LastLocation = FVector::ZeroVector;
	int32 BlockedTicks = 0;
	FDelegateHandle BlockedHandle;
};
