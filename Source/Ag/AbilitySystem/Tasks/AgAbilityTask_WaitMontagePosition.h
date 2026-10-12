// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AgAbilityTask_WaitMontagePosition.generated.h"

class UAnimMontage;

/**
 * 재생 중인 몽타주가 정한 위치(초)에 닿을 때까지 기다립니다(구현 계획서 '기획서에 초로 적힌 구간').
 * 몽타주 재생 위치로 재므로 히트스톱 중에는 함께 멈추고, 재생 속도에 따라 빨라지거나 느려집니다.
 * 몽타주가 재생 중이 아니면 바로 알립니다.
 */
UCLASS()
class UAgAbilityTask_WaitMontagePosition : public UAbilityTask
{
	GENERATED_BODY()

public:
	UAgAbilityTask_WaitMontagePosition();

	UPROPERTY(BlueprintAssignable)
	FGenericGameplayTaskDelegate OnReached;

	static UAgAbilityTask_WaitMontagePosition* WaitMontagePosition(UGameplayAbility* OwningAbility, UAnimMontage* Montage, float Position);

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;

private:
	void CheckPosition();

	TWeakObjectPtr<UAnimMontage> Montage;
	float Position = 0.f;
};
