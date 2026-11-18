// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Tasks/AgAbilityTask_ApplyEffect.h"
#include "AgAbilityTask_PerfectDodgeWindow.generated.h"

class UBoxComponent;

USTRUCT()
struct FAgPerfectDodgeWindowParams : public FAgApplyEffectParams
{
	GENERATED_BODY()

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

// 극한 회피가 발동해 구간 효과가 지워지면 부모가 Task를 끝내므로, 상자도 그때 함께 사라져 남은 공격을 먼 거리에서
// 맞지 않습니다.
UCLASS()
class UAgAbilityTask_PerfectDodgeWindow : public UAgAbilityTask_ApplyEffect
{
	GENERATED_BODY()
	friend struct FAgPerfectDodgeWindowParams;

public:
	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	UPROPERTY()
	TObjectPtr<UBoxComponent> Box;
};
