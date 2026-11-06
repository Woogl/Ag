// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpecHandle.h"
#include "Tasks/StateTreeAITask.h"
#include "AgEnemyAITasks.generated.h"

class AAgEnemyAIController;

USTRUCT()
struct FAgEnemyAITask_KeepDistanceInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAgEnemyAIController> Controller = nullptr;

	/** 마지막 확인에서 정한 대상과의 목표 거리(캡슐 중심 수평)입니다. */
	FFloatInterval TargetRange = FFloatInterval(0.f, 0.f);
	float CheckElapsed = 0.f;
	float StrafeDirection = 1.f;
	bool bApproaching = false;
	bool bWasDelaying = false;
};

/**
 * 쓸 수 있는 패턴이 생길 때까지 목표 거리 안으로 움직이고, 생기면 끝납니다. 공격할 차례(후딜레이가 없고, 동시 공격 수에서 차례를
 * 받았고, 쿨다운이 끝난 패턴이 있음)면 그 패턴들의 발동 가능 거리, 아니면 적정 거리가 목표입니다.
 */
USTRUCT(meta = (DisplayName = "적: 간격 지키기", Category = "Ag|Enemy AI"))
struct FAgEnemyAITask_KeepDistance : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAgEnemyAITask_KeepDistanceInstanceData;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

USTRUCT()
struct FAgEnemyAITask_RunPatternInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAgEnemyAIController> Controller = nullptr;

	/** 가중치(`Weight`) 대신 반응 확률(`ResponseChance`)이 있는 반응형 패턴 중에서 고릅니다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bResponse = false;

	/** 코드는 읽지 않고, StateTree 디버거에서 고른 패턴을 보려고 둡니다. */
	UPROPERTY(VisibleAnywhere, Category = "Output")
	FName RunningPattern;

	FGameplayAbilitySpecHandle Handle;
};

USTRUCT(meta = (DisplayName = "적: 패턴 실행", Category = "Ag|Enemy AI"))
struct FAgEnemyAITask_RunPattern : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAgEnemyAITask_RunPatternInstanceData;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

USTRUCT()
struct FAgEnemyAITask_PhaseTransitionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAgEnemyAIController> Controller = nullptr;
};

USTRUCT(meta = (DisplayName = "적: 페이즈 전환", Category = "Ag|Enemy AI"))
struct FAgEnemyAITask_PhaseTransition : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAgEnemyAITask_PhaseTransitionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

USTRUCT()
struct FAgEnemyAITask_WaitReactionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAgEnemyAIController> Controller = nullptr;

	/** 머무는 동안 움찔이 아닌 피격 반응(넉업·넉다운·그로기·처형 피격)이 있었습니다. */
	bool bHeavyReaction = false;
};

/** 피격 반응이 끝날 때까지 기다립니다. 움찔이 아닌 반응이 있었으면 끝날 때 패턴 후딜레이를 겁니다. */
USTRUCT(meta = (DisplayName = "적: 피격 반응 대기", Category = "Ag|Enemy AI"))
struct FAgEnemyAITask_WaitReaction : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAgEnemyAITask_WaitReactionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

USTRUCT()
struct FAgEnemyAITask_NonCombatInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAgEnemyAIController> Controller = nullptr;
};

USTRUCT(meta = (DisplayName = "적: 비전투", Category = "Ag|Enemy AI"))
struct FAgEnemyAITask_NonCombat : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	FAgEnemyAITask_NonCombat();

	using FInstanceDataType = FAgEnemyAITask_NonCombatInstanceData;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

USTRUCT()
struct FAgEnemyAITask_CombatInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAgEnemyAIController> Controller = nullptr;
};

USTRUCT(meta = (DisplayName = "적: 전투", Category = "Ag|Enemy AI"))
struct FAgEnemyAITask_Combat : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	FAgEnemyAITask_Combat();

	using FInstanceDataType = FAgEnemyAITask_CombatInstanceData;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

private:
	static void UpdateTarget(AAgEnemyAIController& Controller);
};

USTRUCT()
struct FAgEnemyAITask_PatrolInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAgEnemyAIController> Controller = nullptr;

	/** 정찰 지점의 순서입니다. 정찰 지점이 없거나 모두 갈 수 없으면 처음 자리로 갑니다. */
	int32 PointIndex = INDEX_NONE;
	/** 도착한 뒤로 연달아 이동 요청에 실패한 정찰 지점 수입니다. */
	int32 FailedCount = 0;
	bool bMoveRequested = false;
};

/** 정찰 지점을 순서대로 돌고, 갈 수 없는 지점은 건너뜁니다. 정찰 지점이 없거나 모두 갈 수 없으면 처음 자리로 돌아가 처음 방향을 바라봅니다. */
USTRUCT(meta = (DisplayName = "적: 정찰", Category = "Ag|Enemy AI"))
struct FAgEnemyAITask_Patrol : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAgEnemyAITask_PatrolInstanceData;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
