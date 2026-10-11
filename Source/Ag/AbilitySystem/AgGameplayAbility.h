// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "AgGameplayAbility.generated.h"

class AAgCharacterBase;
class UAgAbilityTask_HoldTag;
class UAgAttributeSet;
class UAgCharacterData;

/** 어빌리티를 시작할 때 치르는 자원(양수) */
struct FAgAbilityCost
{
	float SP = 0.f;
	float MP = 0.f;
	float UP = 0.f;
};

/**
 * 모든 Ag 어빌리티의 부모. 어빌리티는 모두 이 클래스를 바로 상속하고, 여러 어빌리티가 함께 쓰는 동작은 어빌리티 태스크로
 * 둡니다(구현 계획서 '어빌리티 구조'). 주인의 캐릭터 데이터를 읽고, 쿨다운과 비용, 모션 캔슬을 위한 행동 중(State.Busy)을
 * 다룹니다. 행동은 후딜이 시작될 때까지 State.Busy를 들고 있고, 그동안 새 행동은 시작하지 못합니다.
 */
UCLASS(Abstract)
class UAgGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UAgGameplayAbility();

	/** 시작할 때 치르는 자원(양수). 비용을 치르는 어빌리티의 비용 GE(UAgEffect_Cost)가 읽습니다. */
	virtual FAgAbilityCost GetCost() const;

protected:
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// 쿨다운과 비용: CanActivateAbility가 확인하고 CommitAbility가 치릅니다. 보스 패턴은 패턴 ID마다 쿨다운 태그(DA_Boss)와
	// 시간(Boss.json)이 다르고, 패턴 시작부터 셉니다. 비용은 비용 GE(CostGameplayEffectClass)로 치르고, 치를 수 있는지는
	// 행동마다의 규칙(CanPayCost)이 정합니다.
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual bool CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** The tag this ability's cooldown gives; the ability can't start while the owner has it. */
	void SetCooldownTag(const FGameplayTag& CooldownTag);

	/** 쿨다운 length from the character data; 0 for none. */
	virtual float GetCooldownDuration() const { return 0.f; }

	/** Whether the owner has enough of the resources to start (each action's own rule). */
	virtual bool CanPayCost(const UAgAttributeSet& Stats) const { return true; }

	/**
	 * 플레이어 행동의 태그(플레이어 입력 '상태별 허용 입력', '모션 캔슬'): 다른 행동의 행동 중, 피격 반응, 그로기, 처형, 사망 중에는
	 * 시작하지 못하고, 시작하면 진행 중인 행동과 달리기를 취소합니다. bIgnoresMovement면 State.Acting을 가져 도는 동안 이동 입력을 무시합니다.
	 */
	void SetupPlayerAction(const FGameplayTag& AbilityTag, bool bIgnoresMovement = true);

	/**
	 * 피격 반응(넉업, 넉백, 가드 밀림, 가드 브레이크)의 태그와 발동 이벤트(전투 시스템 '피격 반응', '상태 우선순위'): TriggerEvent로
	 * 시작하고, 같은 우선순위의 새 반응은 앞 반응을 대신합니다. 하던 행동, 달리기, 보스 패턴을 멈추고, 그로기·처형·사망 중에는 시작하지 못합니다.
	 * AbilityTag는 Ability.Reaction.HitReaction 아래의 태그입니다.
	 */
	void SetupHitReaction(const FGameplayTag& AbilityTag, const FGameplayTag& TriggerEvent);

	/** 보스 패턴의 태그: 피격 반응, 그로기, 처형 피격, 사망, 비전투 중에는 시작하지 못합니다. */
	void SetupBossPattern();

	AAgCharacterBase* GetAgCharacter() const;

	template <typename TData>
	const TData* GetCharacterData() const
	{
		return Cast<TData>(GetCharacterDataBase());
	}

	/** 행동 중(State.Busy)을 시작합니다. */
	void BeginBusy();

	/** 행동 중을 끝냅니다. 후딜이 시작되고, 다른 행동이 이 행동을 취소할 수 있습니다. */
	void EndBusy();

	/** 행동 중인지. 실행 중인 행동은 이것이 풀린 뒤가 후딜입니다. */
	bool IsBusy() const { return BusyTag != nullptr; }

	/** Ends this ability from a callback, cancelled or not. */
	void EndSelf(bool bWasCancelled);

private:
	const UAgCharacterData* GetCharacterDataBase() const;

	FGameplayTagContainer CooldownTags;

	/** 행동 중인 동안 State.Busy를 붙여 둡니다. 어빌리티가 끝나면 함께 끝납니다. */
	UPROPERTY()
	TObjectPtr<UAgAbilityTask_HoldTag> BusyTag;
};
