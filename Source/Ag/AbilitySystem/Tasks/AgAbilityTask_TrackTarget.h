// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_TrackTarget.generated.h"

USTRUCT()
struct FAgTrackTargetParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	TOptional<float> ReleaseRange;

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

/** 어빌리티의 행동 대상을 붙잡아 둡니다. 대상이 죽으면 다시 고르고, 해제 거리 밖으로 나가거나 고를 대상이 없으면 끝납니다. */
UCLASS()
class UAgAbilityTask_TrackTarget : public UAbilityTask
{
	GENERATED_BODY()
	friend struct FAgTrackTargetParams;

public:
	UAgAbilityTask_TrackTarget();

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;

private:
	/** 대상이 죽었으면 다시 고르고, 붙잡을 대상이 남았는지 돌려줍니다. */
	bool UpdateTarget();

	UPROPERTY()
	FAgTrackTargetParams Params;

	float ReleaseRange = 0.f;
};
