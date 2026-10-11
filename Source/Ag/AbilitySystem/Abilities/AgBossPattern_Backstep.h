// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgBossPattern_Backstep.generated.h"

/**
 * B1 백스텝: 모션 자체의 루트 모션으로 물러납니다. 공격 패턴이 아니라서 모션 재생 속도는 받지 않지만, 2페이즈 패턴 진행
 * 속도는 받습니다. 정지 간격을 두지 않습니다. AI가 플레이어의 평타에 맞춰 시작합니다.
 */
UCLASS()
class UAgBossPattern_Backstep : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgBossPattern_Backstep();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	UFUNCTION()
	void HandleMontageFinished();

	UFUNCTION()
	void HandleMontageCancelled();
};
