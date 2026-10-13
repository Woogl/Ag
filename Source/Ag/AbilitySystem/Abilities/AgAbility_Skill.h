// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_Skill.generated.h"

/** 스킬의 전용 값 (플레이어 사양 '스킬'): 공격 데이터와 MP 소모 */
USTRUCT()
struct FAgSkillParams : public FAgPlayerAttack
{
	GENERATED_BODY()

	/** MP 소모 */
	UPROPERTY(EditAnywhere, Category = "Skill", meta = (ClampMin = 0))
	float MPCost = 0.f;
};

/** 스킬 (돌진 베기): MP를 쓰고 쿨다운이 있는 공격입니다. */
UCLASS()
class UAgAbility_Skill : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgAbility_Skill();

	virtual const UScriptStruct* GetAbilityParamsType() const override { return FAgSkillParams::StaticStruct(); }
	virtual FAgAbilityCost GetCost() const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual bool CanPayCost(const UAgAttributeSet& Stats) const override;

private:
	UFUNCTION()
	void HandleRecoveryStarted();

	UFUNCTION()
	void HandleAttackCompleted();

	UFUNCTION()
	void HandleAttackInterrupted();
};
