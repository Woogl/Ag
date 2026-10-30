// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Data/AgAbilityData.h"
#include "AgGameplayAbility.generated.h"

class UGameplayEffect;
enum class EAgEffect : uint8;
struct FAgDamage;
struct FAgHitRewardState;

UCLASS()
class UAgGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UAgGameplayAbility();
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual bool CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual bool DoesAbilitySatisfyTagRequirements(const UAbilitySystemComponent& ASC, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void GetCooldownTimeRemainingAndDuration(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, float& TimeRemaining, float& CooldownDuration) const override;

	FAgResourceCost GetCost() const;

	/** 엔진 MakeOutgoingGameplayEffectSpec은 GE 클래스를 받는데 Effects.json의 효과는 GE 객체이므로 직접 만듭니다. */
	FGameplayEffectSpecHandle MakeEffectSpec(const UGameplayEffect& Effect) const;
	FGameplayEffectSpecHandle MakeDamageSpec(const FAgDamage& Damage, const TSharedPtr<FAgHitRewardState>& Reward) const;

	FName GetAbilityId() const
	{
		return AbilityId;
	}

	const FAgAbilityData& GetDefinition() const
	{
		return Definition;
	}

	/** 이벤트로 발동하지 않았으면 비어 있습니다. */
	const FGameplayTag& GetTriggerEventTag() const
	{
		return TriggerEventTag;
	}

	AActor* GetActionTarget() const
	{
		return ActionTarget.Get();
	}

	AActor* GetTargetingTarget() const
	{
		return Definition.Targeting ? ActionTarget.Get() : nullptr;
	}

	float GetTargetingRange() const
	{
		return Definition.Targeting ? Definition.Targeting->Range : 0.f;
	}

	float GetTargetSpacing() const
	{
		return Definition.Targeting ? Definition.Targeting->TargetSpacing : 0.f;
	}

	/** 기본 공격처럼 다시 재생할 때마다 대상을 고르는 행동을 위해 재생 Task가 부릅니다. */
	void RefreshTarget();

	virtual void OnGameplayTaskActivated(UGameplayTask& Task) override;
	virtual void OnGameplayTaskDeactivated(UGameplayTask& Task) override;

protected:
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual bool CheckCost(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCost(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual bool CheckCooldown(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCooldown(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo) const override;

private:
	const FAgAbilityData* FindDefinition(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const;
	FGameplayEffectSpecHandle MakeEffectSpec(EAgEffect Effect) const;
	FGameplayEffectSpecHandle MakeEffectSpec(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const UGameplayEffect* Effect) const;

	AActor* FindTargetingTarget(const FAgAbilityData& Data, const FGameplayAbilityActorInfo* ActorInfo) const;

	bool IsEventTrigger(const FGameplayEventData* TriggerEventData) const;

	void FinishIfTasksDone();

	FName AbilityId;

	/**
	 * 데이터를 다시 읽어도 이 실행이 끝날 때까지 같은 값을 쓰도록 부여할 때와 발동할 때마다 복사해 둡니다.
	 * 태그와 Activation은 부여할 때 스펙과 엔진 속성에 넣으므로 이 둘이 바뀐 정의로는 발동하지 않아, 사본에는 늘
	 * 부여 당시 값이 남습니다.
	 */
	UPROPERTY()
	FAgAbilityData Definition;

	TWeakObjectPtr<AActor> ActionTarget;

	FGameplayTag TriggerEventTag;

	bool bStartingTasks = false;
};
