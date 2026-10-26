// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/AgAbilityData.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "Tasks/StateTreeAITask.h"
#include "AgEnemyAITasks.generated.h"

class AAgEnemyAIController;
class UAgCharacterData;

USTRUCT()
struct FAgEnemyAITask_PostPatternDelayInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAgEnemyAIController> Controller = nullptr;

	float Duration = 0.f;
	float Elapsed = 0.f;

	float StrafeDirection = 1.f;
};

/** 패턴 후딜레이 동안 대상 주위를 옆으로 걷습니다. */
USTRUCT(meta = (DisplayName = "적: 패턴 후딜레이", Category = "Ag|Enemy AI"))
struct FAgEnemyAITask_PostPatternDelay : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAgEnemyAITask_PostPatternDelayInstanceData;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

USTRUCT()
struct FAgEnemyAITask_ApproachInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAgEnemyAIController> Controller = nullptr;

	float Elapsed = 0.f;
};

USTRUCT(meta = (DisplayName = "적: 다가가기", Category = "Ag|Enemy AI"))
struct FAgEnemyAITask_Approach : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAgEnemyAITask_ApproachInstanceData;
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

	UPROPERTY(EditAnywhere, Category = "Parameter")
	FName Pattern;

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

private:
	static void GatherCandidates(const UAgCharacterData& Data, float Distance, int32 Phase, TFunctionRef<bool(FName)> IsReady, TArray<TPair<FName, float>>& OutCandidates);
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
};

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

	/** 정찰 지점의 순서입니다. 정찰 지점이 없으면 처음 자리로 갑니다. */
	int32 PointIndex = INDEX_NONE;
	bool bMoveRequested = false;
};

/** 정찰 지점을 순서대로 돕니다. 정찰 지점이 없으면 처음 자리로 돌아가 처음 방향을 바라봅니다. */
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
