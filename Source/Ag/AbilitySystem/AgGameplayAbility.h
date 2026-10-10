// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "AgGameplayAbility.generated.h"

class AAgCharacterBase;
class UAgCharacterData;

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

	/**
	 * Sets the tags of a player action (플레이어 사양 '입력과 모션 캔슬'):
	 * the action owns State.Acting while it runs, can't start during another action's 행동 중, a hit reaction, groggy,
	 * execution or death, and cancels the action it interrupts.
	 */
	void SetupPlayerAction(const FGameplayTag& AbilityTag);

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

	bool bBusy = false;
};
