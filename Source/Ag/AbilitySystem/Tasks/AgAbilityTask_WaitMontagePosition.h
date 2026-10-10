// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AgAbilityTask_WaitMontagePosition.generated.h"

class UAnimMontage;

/**
 * Waits until a playing montage reaches a time (기획서에 초로 적힌 구간: 회피 무적, 극한 회피 구간, 후딜 시작).
 * It reads the montage's play position, so hitstop pauses the wait and the play rate scales it.
 * Fires at once when the montage is no longer playing.
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
