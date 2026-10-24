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

USTRUCT(meta = (DisplayName = "적: 움찔 경직 중", Category = "Ag|Enemy AI"))
struct FAgEnemyAICondition_Flinching : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAgEnemyAIConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

	UPROPERTY(EditAnywhere, Category = "Condition")
	bool bInvert = false;
};

/** 패턴 실행 중인지는 이 조건을 둔 상태(패턴 후딜레이, 다가가기, Backstep이 끝난 뒤)가 정하므로 보지 않습니다. */
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

USTRUCT()
struct FAgEnemyAICondition_ResponseReadyInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAgEnemyAIController> Controller = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	FName Pattern;
};

USTRUCT(meta = (DisplayName = "적: 반응 패턴 발동 조건", Category = "Ag|Enemy AI"))
struct FAgEnemyAICondition_ResponseReady : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAgEnemyAICondition_ResponseReadyInstanceData;
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
