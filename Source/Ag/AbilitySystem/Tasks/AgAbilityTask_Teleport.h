// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgTaskParams.h"
#include "GameplayTagContainer.h"
#include "AgAbilityTask_Teleport.generated.h"

struct FGameplayEventData;

USTRUCT()
struct FAgTeleportParams : public FAgTaskParams
{
	GENERATED_BODY()

	/** 이 어빌리티의 공격이 일으킨 이 이벤트를 받으면 이벤트의 상대(Instigator) 등 뒤로 순간이동합니다. */
	UPROPERTY()
	FGameplayTag Event;

	/** 나타날 때 상대와의 캡슐 중심 사이 수평 거리입니다. */
	UPROPERTY()
	float Distance = 0.f;

	/** 정후방에 나타날 수 없으면 좌우로 이 각도(도)까지 찾습니다. */
	UPROPERTY()
	float MaxAngle = 0.f;

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

/** 순간이동하면 행동 대상을 상대로 바꾸고 Event.Teleported를 보낸 뒤 끝나며, 나타날 수 없으면 구간 끝까지 다음 이벤트를 기다립니다. */
UCLASS()
class UAgAbilityTask_Teleport : public UAbilityTask
{
	GENERATED_BODY()
	friend struct FAgTeleportParams;

public:
	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	void HandleEvent(const FGameplayEventData* Payload);

	UPROPERTY()
	FAgTeleportParams Params;

	FDelegateHandle EventHandle;
};
