// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "AgGameplayAbility.generated.h"

class AAgCharacterBase;
class UAgAttributeSet;
class UAgCharacterData;
struct FAgResourceAmounts;

/**
 * Base of every Ag ability. Reads the owner's character data and handles 행동 중 (State.Busy) for motion cancels:
 * an action holds State.Busy until its recovery (후딜) starts, and a new action can't start while it is held.
 */
UCLASS(Abstract)
class UAgGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UAgGameplayAbility();

protected:
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// 쿨다운 and resource cost, checked by CanActivateAbility and paid by CommitAbility.
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

	/** The tag this ability's cooldown gives; the ability can't start while the owner has it. */
	void SetCooldownTag(const FGameplayTag& CooldownTag);

	/** 쿨다운 length from the character data; 0 for none. */
	virtual float GetCooldownDuration() const { return 0.f; }

	/** Whether the owner has enough of the resources to start (each action's own rule). */
	virtual bool CanPayCost(const UAgAttributeSet& Stats) const { return true; }

	/** The resources spent on start, as positive amounts. */
	virtual FAgResourceAmounts GetCost() const;

	/**
	 * Sets the tags of a player action (플레이어 사양 '입력과 모션 캔슬'):
	 * the action can't start during another action's 행동 중, a hit reaction, groggy, execution or death, and cancels
	 * the action it interrupts. With bIgnoresMovement it owns State.Acting, so movement input is ignored while it runs.
	 */
	void SetupPlayerAction(const FGameplayTag& AbilityTag, bool bIgnoresMovement = true);

	AAgCharacterBase* GetAgCharacter() const;

	template <typename TData>
	const TData* GetCharacterData() const
	{
		return Cast<TData>(GetCharacterDataBase());
	}

	/** Starts 행동 중. */
	void BeginBusy();

	/** Ends 행동 중: the recovery (후딜) starts and other actions may cancel this one. */
	void EndBusy();

	/** Ends this ability from a callback, cancelled or not. */
	void EndSelf(bool bWasCancelled);

private:
	const UAgCharacterData* GetCharacterDataBase() const;

	FGameplayTagContainer CooldownTags;

	bool bBusy = false;
};
