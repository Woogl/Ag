// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_JumpAttack.generated.h"

/** 점프 공격의 전용 값 (플레이어 사양 '점프 공격'): 공격 데이터와 내리꽂는 속도 */
USTRUCT()
struct FAgJumpAttackParams : public FAgPlayerAttack
{
	GENERATED_BODY()

	/** 내리꽂기: 내려베기가 끝난 뒤 떨어지는 속도 */
	UPROPERTY(EditAnywhere, Category = "Jump Attack", meta = (Units = "cm/s", ClampMin = 0))
	float PlungeSpeed = 0.f;
};

/**
 * 점프 공격: 점프해 공중에 있을 때 평타 키로 씁니다. 내려벤 뒤 아래로 빠르게 떨어지고, 착지하며 바닥을 찍습니다.
 * 판정 구간 하나가 내려베기부터 착지해 바닥을 찍을 때까지 이어지고, 후딜은 착지하고 정해진 시간이 지나면 시작합니다.
 * 공중에서는 점프가 행동 중이므로, 행동 중이 아니라 점프 중(State.Jumping)인지로 시작할 수 있는지 정하고 점프를 취소합니다.
 */
UCLASS()
class UAgAbility_JumpAttack : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgAbility_JumpAttack();

	virtual const UScriptStruct* GetActionParamsType() const override { return FAgJumpAttackParams::StaticStruct(); }

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	/** 내려베기가 끝났습니다(몽타주 노티파이). 아래로 빠르게 떨어집니다. */
	UFUNCTION()
	void HandlePlunge(FGameplayEventData Payload);

	/** 착지했습니다. 내려베기 중이어도 바닥 찍기로 넘어갑니다. */
	UFUNCTION()
	void HandleLanded(EMovementMode NewMovementMode);

	UFUNCTION()
	void HandleRecoveryStarted();

	UFUNCTION()
	void HandleAttackCompleted();

	UFUNCTION()
	void HandleAttackInterrupted();
};
