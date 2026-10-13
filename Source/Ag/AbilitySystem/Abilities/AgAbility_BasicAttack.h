// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_BasicAttack.generated.h"

class UAgAbilityTask_Attack;

/** 평타의 전용 값 (플레이어 사양 '평타') */
USTRUCT()
struct FAgBasicAttackParams : public FAgActionParams
{
	GENERATED_BODY()

	/** 타마다 하나씩, 1타부터. 몽타주는 DA_Player의 BasicAttackMontages에서 같은 순서로 찾습니다. */
	UPROPERTY(EditAnywhere, Category = "Basic Attack")
	TArray<FAgPlayerAttack> Combo;
};

/**
 * 평타: 전용 값의 Combo 항목마다 한 타인 연속 공격이며, 타마다 하나의 행동입니다.
 * 한 타의 후딜에 평타 키를 누르면 다음 타를 시작합니다(마지막 타 다음은 첫 타). 입력 없이 후딜이 끝나거나, 다른 행동이나
 * 피격 반응이 끊으면 처음 타로 돌아갑니다.
 */
UCLASS()
class UAgAbility_BasicAttack : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgAbility_BasicAttack();

	virtual const UScriptStruct* GetActionParamsType() const override { return FAgBasicAttackParams::StaticStruct(); }

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	/** Index번째 타를 시작합니다. 앞 타의 몽타주는 멈추지 않고 이번 타의 몽타주에 섞입니다. */
	void StartHit(int32 Index);

	/** 플레이어가 평타 키를 다시 눌렀습니다(이 어빌리티가 실행 중일 때 플레이어 캐릭터가 보냄). */
	UFUNCTION()
	void HandleAttackInput(FGameplayEventData Payload);

	UFUNCTION()
	void HandleRecoveryStarted();

	UFUNCTION()
	void HandleAttackCompleted();

	UFUNCTION()
	void HandleAttackInterrupted();

	UPROPERTY()
	TObjectPtr<UAgAbilityTask_Attack> AttackTask;

	int32 HitIndex = 0;
};
