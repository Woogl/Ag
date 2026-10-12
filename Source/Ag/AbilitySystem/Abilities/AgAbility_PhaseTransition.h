// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_PhaseTransition.generated.h"

/**
 * 페이즈 전환 (보스 사양 '페이즈 전환'): 2페이즈 HP 구간에 들어선 뒤 패턴, 넉백, 그로기 중이 아닐 때 보스 AI가 시작합니다.
 * 포효하는 동안 무적이고, 포효가 끝나면 PP가 가득 차고 2페이즈가 시작됩니다.
 */
UCLASS()
class UAgAbility_PhaseTransition : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgAbility_PhaseTransition();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	UFUNCTION()
	void HandleRoarFinished();

	UFUNCTION()
	void HandleRoarCancelled();
};
