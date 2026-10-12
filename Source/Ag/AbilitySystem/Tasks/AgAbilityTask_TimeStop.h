// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AgAbilityTask_TimeStop.generated.h"

class UAgTimeSubsystem;

/**
 * 도는 동안 주인만 빼고 게임 전체의 시간을 멈춥니다(구현 계획서 '시간 정지', '궁극기'). 끝나면(어빌리티가 끝날 때 포함)
 * 시간 정지를 끕니다.
 */
UCLASS()
class UAgAbilityTask_TimeStop : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UAgAbilityTask_TimeStop* TimeStop(UGameplayAbility* OwningAbility);

	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	TWeakObjectPtr<UAgTimeSubsystem> TimeSubsystem;
};
