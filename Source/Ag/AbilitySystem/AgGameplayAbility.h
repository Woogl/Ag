// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Data/AgAbilityData.h"
#include "AgGameplayAbility.generated.h"

class UGameplayEffect;
enum class EAgEffect : uint8;
struct FAgDamage;
struct FAgHitState;

UCLASS()
class UAgGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UAgGameplayAbility();
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual bool CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual bool DoesAbilitySatisfyTagRequirements(const UAbilitySystemComponent& ASC, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual bool CheckCost(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** 남은 충전 수입니다. 걸린 쿨다운 효과 하나가 쓴 충전 하나입니다. */
	int32 GetCharges() const;

	/** 다음 충전까지 남은 시간과 쿨다운 길이입니다. 충전이 남아 있어도 값을 줍니다. */
	void GetRechargeTimeRemainingAndDuration(float& TimeRemaining, float& CooldownDuration) const;

	/**
	 * 엔진 MakeOutgoingGameplayEffectSpec은 GE 클래스를 받는데 Effects.json의 효과는 GE 객체이므로 직접 만듭니다. 부여된
	 * 인스턴스에서 부르며 늘 유효한 스펙을 줍니다.
	 */
	FGameplayEffectSpecHandle MakeEffectSpec(const UGameplayEffect& Effect) const;
	FGameplayEffectSpecHandle MakeDamageSpec(const FAgDamage& Damage, const TSharedPtr<FAgHitState>& HitState) const;

	FName GetAbilityId() const
	{
		return AbilityId;
	}

	const FAgAbilityData& GetDefinition() const
	{
		return Definition;
	}

	/** 캐릭터 데이터에 있는 지금의 정의입니다. 데이터를 다시 읽은 뒤에는 사본(GetDefinition)과 다를 수 있고, 받은 포인터는 그 함수 안에서만 씁니다. */
	const FAgAbilityData* FindDefinition() const;

	/** 이 발동을 일으킨 게임플레이 이벤트입니다. 이벤트 트리거로 발동하지 않았으면 없습니다. */
	const FGameplayEventData* GetTriggerEvent() const;

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

	void RefreshTarget();

	void SetActionTarget(AActor* NewTarget);

	virtual void OnGameplayTaskActivated(UGameplayTask& Task) override;
	virtual void OnGameplayTaskDeactivated(UGameplayTask& Task) override;

protected:
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void ApplyCost(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual bool CheckCooldown(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCooldown(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo) const override;

private:
	FGameplayEffectSpecHandle MakeEffectSpec(EAgEffect Effect) const;

	AActor* FindTargetingTarget(const FAgAbilityData& Data) const;

	void FinishIfTasksDone();

	FName AbilityId;

	/**
	 * 데이터를 다시 읽어도 이 실행이 끝날 때까지 같은 값을 쓰도록 부여할 때와 발동할 때마다 복사해 둡니다.
	 * 태그와 발동 조건은 부여할 때 스펙과 엔진 속성에 넣으므로 이 둘이 바뀐 정의로는 발동하지 않아, 사본에는 늘
	 * 부여 당시 값이 남습니다.
	 */
	UPROPERTY()
	FAgAbilityData Definition;

	TWeakObjectPtr<AActor> ActionTarget;

	bool bStartingTasks = false;
};
