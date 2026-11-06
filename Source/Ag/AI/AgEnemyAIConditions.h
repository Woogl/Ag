// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Conditions/StateTreeAIConditionBase.h"
#include "AgEnemyAIConditions.generated.h"

class AAgEnemyAIController;

USTRUCT()
struct FAgEnemyAIConditionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAgEnemyAIController> Controller = nullptr;
};

USTRUCT(meta = (DisplayName = "적: 사망", Category = "Ag|Enemy AI"))
struct FAgEnemyAICondition_Dead : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAgEnemyAIConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

USTRUCT(meta = (DisplayName = "적: 플레이어 사망", Category = "Ag|Enemy AI"))
struct FAgEnemyAICondition_PlayerDead : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAgEnemyAIConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

USTRUCT(meta = (DisplayName = "적: 피격 반응 중", Category = "Ag|Enemy AI"))
struct FAgEnemyAICondition_Reacting : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAgEnemyAIConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

/** 전투 서브시스템이 패턴을 막은 동안(플레이어가 처형하는 동안) 참입니다. 패턴 실행 상태에서 빠져나가 패턴을 멈출 때 씁니다. */
USTRUCT(meta = (DisplayName = "적: 패턴 금지", Category = "Ag|Enemy AI"))
struct FAgEnemyAICondition_PatternsBlocked : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAgEnemyAIConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

/** 패턴 실행 중인지는 이 조건을 둔 상태(간격 지키기, 반응형 패턴이 끝난 뒤)가 정하므로 보지 않습니다. */
USTRUCT(meta = (DisplayName = "적: 페이즈 전환 시작 조건", Category = "Ag|Enemy AI"))
struct FAgEnemyAICondition_PhaseTransitionReady : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAgEnemyAIConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

/** 패턴 후딜레이 중에만 참입니다. 평가할 때마다 발동 확률을 굴리므로, 한 번만 평가하는 이벤트 전환(기본 공격 시작)에만 둡니다. */
USTRUCT(meta = (DisplayName = "적: 반응형 패턴 발동 조건", Category = "Ag|Enemy AI"))
struct FAgEnemyAICondition_ResponseReady : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAgEnemyAIConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

USTRUCT(meta = (DisplayName = "적: 대상을 알아차림", Category = "Ag|Enemy AI"))
struct FAgEnemyAICondition_TargetNoticed : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAgEnemyAIConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

USTRUCT(meta = (DisplayName = "적: 대상을 놓침", Category = "Ag|Enemy AI"))
struct FAgEnemyAICondition_TargetLost : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAgEnemyAIConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
