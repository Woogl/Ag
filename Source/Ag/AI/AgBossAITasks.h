// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/AgAbilityData.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "Tasks/StateTreeAITask.h"
#include "AgBossAITasks.generated.h"

class AAgBossAIController;

/**
 * 보스 AI(ST_Boss)의 태스크. 흐름(어느 상태에서 어디로 가는지)은 에셋에 있고, 태스크는 그 상태에서 할 일만 맡습니다.
 * 수치는 에셋에 적지 않고 보스 데이터(Characters.json의 Boss)에서 읽습니다.
 */

/** 보스 사양 '패턴 후딜레이 대기 시간'의 두 경우 */
UENUM()
enum class EAgBossWait : uint8
{
	/** 레벨 시작, 넉백, 페이즈 전환, 그로기 유지, 처형 피격 뒤: 고정 대기 시간 */
	Fixed,
	/** 공격 패턴 실행 뒤: 페이즈별 범위에서 무작위 */
	AfterPattern,
};

USTRUCT()
struct FAgBossAITask_WaitInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAgBossAIController> Controller = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	EAgBossWait Wait = EAgBossWait::Fixed;

	float Duration = 0.f;
	float Elapsed = 0.f;

	/** 좌우 걷기: +1이면 오른쪽, -1이면 왼쪽 */
	float StrafeDirection = 1.f;
};

/** 패턴 후딜레이: 대기 시간 동안 플레이어 주위를 좌우로 걷고 성공합니다. 방향은 시작할 때마다 무작위로 고릅니다. */
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

/** 보스 사양 '패턴 선택' 3: 후보가 없으면 AI의 대상(플레이어)에게 걸어가고, 재시도 간격이 지나면 성공합니다. */
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

	/** 실행할 패턴의 ID. 비워 두면 보스 사양 '패턴 선택'대로 후보 중 가중치로 고릅니다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FName Pattern;

	/** 실행 중인 패턴의 ID. StateTree 디버거에서 고른 패턴을 볼 수 있습니다. */
	UPROPERTY(VisibleAnywhere, Category = "Output")
	FName RunningPattern;

	FGameplayAbilitySpecHandle Handle;
};

/**
 * 패턴 실행: 패턴을 고르고 그 어빌리티를 시작해, 어빌리티가 끝나면 성공합니다.
 * 후보가 없거나 시작하지 못하면 실패합니다(보스 사양 '패턴 선택' 3).
 */
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
	/** 가중치로 하나를 고릅니다. Random01은 [0, 1). 모든 가중치가 0이면 INDEX_NONE입니다. */
	static int32 PickWeighted(TConstArrayView<float> Weights, float Random01);

	/**
	 * 보스 사양 '패턴 선택' 1: 어빌리티 목록에서 지금 쓸 수 있는 패턴(전용 값이 패턴 공통 값이고, 그 거리 구간의 가중치가
	 * 0보다 크고, 페이즈가 허용하고, IsReady가 쿨다운이 끝났다고 답함)의 ID와 그 구간의 가중치
	 */
	static void GatherCandidates(TConstArrayView<FName> AbilityIds, float Distance, float NearDistance, float FarDistance, int32 Phase, TFunctionRef<bool(FName)> IsReady, TArray<FName>& OutIds, TArray<float>& OutWeights);
};

USTRUCT()
struct FAgBossAITask_PhaseTransitionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAgBossAIController> Controller = nullptr;
};

/** 페이즈 전환: 보스에게 페이즈 전환 이벤트를 보내고, 페이즈 전환 어빌리티가 끝나면 성공합니다. 시작하지 못하면 실패합니다. */
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

/** 피격 반응: 넉백과 그로기(처형 피격 포함)가 모두 끝나면 성공합니다. 그동안 AI는 아무것도 하지 않습니다. */
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

/**
 * 비전투(보스 사양 '전투 시작과 행동 전환' 2): 진행 중인 패턴을 멈추고 보스에게 비전투 태그를 붙입니다. 끝나지 않습니다.
 * 이 태그가 패턴, 그로기, 페이즈 전환을 막고 보스 회전을 멈춥니다.
 */
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
