// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Conditions/StateTreeAIConditionBase.h"
#include "AgBossAIConditions.generated.h"

class AAgBossAIController;

/** 보스 AI(ST_Boss)의 전환 조건. 수치는 에셋에 적지 않고 보스 데이터(Boss.json)에서 읽습니다. */

USTRUCT()
struct FAgBossAIConditionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAgBossAIController> Controller = nullptr;
};

/** 보스 사양 '전투 시작과 행동 전환' 1: 보스가 사망했습니다. */
USTRUCT(meta = (DisplayName = "보스: 보스 사망", Category = "Ag|Boss AI"))
struct FAgBossAICondition_BossDead : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAgBossAIConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

/** 보스 사양 '전투 시작과 행동 전환' 2: AI의 대상(플레이어)이 사망했습니다. */
USTRUCT(meta = (DisplayName = "보스: 플레이어 사망", Category = "Ag|Boss AI"))
struct FAgBossAICondition_PlayerDead : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAgBossAIConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

/** 보스가 넉백이나 그로기(처형 피격 포함) 중입니다. */
USTRUCT(meta = (DisplayName = "보스: 피격 반응 중", Category = "Ag|Boss AI"))
struct FAgBossAICondition_Reacting : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAgBossAIConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

/**
 * 보스 사양 '전투 시작과 행동 전환' 5: 2페이즈 HP 구간에 들어섰고, 아직 페이즈 전환을 시작하지 않았고, 넉백이나 그로기
 * 중이 아닙니다. 패턴 실행 중이 아닌지는 이 조건을 둔 상태(패턴 후딜레이, 다가가기, 백스텝이 끝난 뒤)가 정합니다.
 */
USTRUCT(meta = (DisplayName = "보스: 페이즈 전환 시작 조건", Category = "Ag|Boss AI"))
struct FAgBossAICondition_PhaseTransitionReady : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAgBossAIConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

USTRUCT()
struct FAgBossAICondition_BackstepReadyInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAgBossAIController> Controller = nullptr;

	/** 백스텝 패턴의 ID. 이 패턴의 쿨다운을 확인합니다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FName Pattern;
};

/** 보스 사양 '백스텝'의 발동 조건: AI의 대상(플레이어)이 가깝고, 백스텝 쿨다운이 끝났고, 확률을 통과했습니다. */
USTRUCT(meta = (DisplayName = "보스: 백스텝 발동 조건", Category = "Ag|Boss AI"))
struct FAgBossAICondition_BackstepReady : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAgBossAICondition_BackstepReadyInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
