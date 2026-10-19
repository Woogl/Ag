// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Conditions/StateTreeAIConditionBase.h"
#include "AgBossAIConditions.generated.h"

class AAgBossAIController;

USTRUCT()
struct FAgBossAIConditionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAgBossAIController> Controller = nullptr;
};

USTRUCT(meta = (DisplayName = "보스: 보스 사망", Category = "Ag|Boss AI"))
struct FAgBossAICondition_BossDead : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAgBossAIConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

USTRUCT(meta = (DisplayName = "보스: 플레이어 사망", Category = "Ag|Boss AI"))
struct FAgBossAICondition_PlayerDead : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAgBossAIConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

USTRUCT(meta = (DisplayName = "보스: 피격 반응 중", Category = "Ag|Boss AI"))
struct FAgBossAICondition_Reacting : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAgBossAIConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

/** 패턴 실행 중인지는 이 조건을 둔 상태(패턴 후딜레이, 다가가기, Backstep이 끝난 뒤)가 정하므로 보지 않습니다. */
USTRUCT(meta = (DisplayName = "보스: 페이즈 전환 시작 조건", Category = "Ag|Boss AI"))
struct FAgBossAICondition_PhaseTransitionReady : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAgBossAIConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

USTRUCT()
struct FAgBossAICondition_BackstepReadyInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAgBossAIController> Controller = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	FName Pattern;
};

USTRUCT(meta = (DisplayName = "보스: Backstep 발동 조건", Category = "Ag|Boss AI"))
struct FAgBossAICondition_BackstepReady : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAgBossAICondition_BackstepReadyInstanceData;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
