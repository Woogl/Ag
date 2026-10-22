// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/AgAbilityData.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "Tasks/StateTreeAITask.h"
#include "AgBossAITasks.generated.h"

class AAgBossAIController;

USTRUCT()
struct FAgBossAITask_WaitInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAgBossAIController> Controller = nullptr;

	float Duration = 0.f;
	float Elapsed = 0.f;

	float StrafeDirection = 1.f;
};

USTRUCT(meta = (DisplayName = "보스: 패턴 후딜레이 대기", Category = "Ag|Boss AI"))
struct FAgBossAITask_Wait : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAgBossAITask_WaitInstanceData;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

USTRUCT()
struct FAgBossAITask_ApproachInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAgBossAIController> Controller = nullptr;

	float Elapsed = 0.f;
};

USTRUCT(meta = (DisplayName = "보스: 다가가기", Category = "Ag|Boss AI"))
struct FAgBossAITask_Approach : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAgBossAITask_ApproachInstanceData;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

USTRUCT()
struct FAgBossAITask_RunPatternInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAgBossAIController> Controller = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	FName Pattern;

	/** 코드는 읽지 않고, StateTree 디버거에서 고른 패턴을 보려고 둡니다. */
	UPROPERTY(VisibleAnywhere, Category = "Output")
	FName RunningPattern;

	FGameplayAbilitySpecHandle Handle;
};

USTRUCT(meta = (DisplayName = "보스: 패턴 실행", Category = "Ag|Boss AI"))
struct FAgBossAITask_RunPattern : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAgBossAITask_RunPatternInstanceData;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

private:
	static void GatherCandidates(TConstArrayView<FName> AbilityIds, float Distance, int32 Phase, TFunctionRef<bool(FName)> IsReady, TArray<TPair<FName, float>>& OutCandidates);
};

USTRUCT()
struct FAgBossAITask_PhaseTransitionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAgBossAIController> Controller = nullptr;
};

USTRUCT(meta = (DisplayName = "보스: 페이즈 전환", Category = "Ag|Boss AI"))
struct FAgBossAITask_PhaseTransition : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAgBossAITask_PhaseTransitionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

USTRUCT()
struct FAgBossAITask_WaitReactionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAgBossAIController> Controller = nullptr;
};

USTRUCT(meta = (DisplayName = "보스: 피격 반응 대기", Category = "Ag|Boss AI"))
struct FAgBossAITask_WaitReaction : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAgBossAITask_WaitReactionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

USTRUCT()
struct FAgBossAITask_NonCombatInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAgBossAIController> Controller = nullptr;
};

USTRUCT(meta = (DisplayName = "보스: 비전투", Category = "Ag|Boss AI"))
struct FAgBossAITask_NonCombat : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	FAgBossAITask_NonCombat();

	using FInstanceDataType = FAgBossAITask_NonCombatInstanceData;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
