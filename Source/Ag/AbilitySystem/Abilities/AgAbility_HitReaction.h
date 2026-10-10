// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_HitReaction.generated.h"

class UAbilityTask_PlayMontageAndWait;

/**
 * 넉백 and 다운 (전투 시스템 '피격 모션'), started by a hit event.
 * Stops the current action, turns toward the attacker and plays the motion; no action is possible until it ends.
 * A new knockback or down restarts the motion with the new reaction.
 */
UCLASS(Abstract)
class UAgAbility_HitReaction : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgAbility_HitReaction();

protected:
	/** Sets the ability tag and the hit event that starts this reaction. */
	void SetupReaction(const FGameplayTag& AbilityTag, const FGameplayTag& TriggerEvent);

	virtual UAnimMontage* GetReactionMontage() const PURE_VIRTUAL(UAgAbility_HitReaction::GetReactionMontage, return nullptr;);

	/** Played after the reaction montage ends, if any (다운: getting up). */
	virtual UAnimMontage* GetFollowUpMontage() const { return nullptr; }

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	void PlayMontage(UAnimMontage* Montage);

	UFUNCTION()
	void HandleMontageFinished();

	UFUNCTION()
	void HandleMontageCancelled();

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	bool bPlayingFollowUp = false;
};

/** 넉백: 뒤로 크게 밀려남. */
UCLASS()
class UAgAbility_KnockBack : public UAgAbility_HitReaction
{
	GENERATED_BODY()

public:
	UAgAbility_KnockBack();

protected:
	virtual UAnimMontage* GetReactionMontage() const override;
};

/** 다운: 쓰러졌다가 일어남. Invincible from the moment the body is on the ground until getting up ends. */
UCLASS()
class UAgAbility_Down : public UAgAbility_HitReaction
{
	GENERATED_BODY()

public:
	UAgAbility_Down();

protected:
	virtual UAnimMontage* GetReactionMontage() const override;
	virtual UAnimMontage* GetFollowUpMontage() const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	UFUNCTION()
	void HandleGrounded(FGameplayEventData Payload);

	bool bInvincible = false;
};
