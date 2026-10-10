// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "Animation/AnimEnums.h"
#include "AgAbility_HitReaction.generated.h"

class UAbilityTask_PlayMontageAndWait;

/**
 * 넉업, 넉백, 다운, 가드 밀림 and 가드 브레이크 (전투 시스템 '피격 모션', '가드 반응'), started by a hit event.
 * Stops the current action, turns toward the attacker and plays the motion; no action is possible until it ends.
 * A new knockup, knockback or down restarts the motion with the new reaction.
 * A motion that marks the moment the body hits the ground (Event.DownGrounded: 다운, 넉업) is invincible from then until
 * the reaction ends.
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

	/** Played after the reaction montage ends, if any (다운, 넉업: getting up). */
	virtual UAnimMontage* GetFollowUpMontage() const { return nullptr; }

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** Plays a montage of this reaction from StartSection (none: the start); the follow-up plays when it ends. */
	void PlayMontage(UAnimMontage* Montage, FName StartSection = NAME_None);

private:
	UFUNCTION()
	void HandleMontageFinished();

	UFUNCTION()
	void HandleMontageCancelled();

	UFUNCTION()
	void HandleGrounded(FGameplayEventData Payload);

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	bool bPlayingFollowUp = false;
	bool bInvincible = false;
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

/** 다운: 쓰러졌다가 일어남. */
UCLASS()
class UAgAbility_Down : public UAgAbility_HitReaction
{
	GENERATED_BODY()

public:
	UAgAbility_Down();

protected:
	virtual UAnimMontage* GetReactionMontage() const override;
	virtual UAnimMontage* GetFollowUpMontage() const override;
};

/**
 * 넉업: 공중으로 떠올랐다가 떨어진 뒤 일어남 (전투 시스템 '넉업').
 * A root motion jump moves the character straight up and down along a parabola of the height and air time, whatever the
 * gravity. The motions' own root motion is ignored meanwhile, so the landing plays where the character fell. The rising
 * pose blends into the falling pose around the top of the flight; landing plays the fall montage's Land section, then the
 * 다운 get-up.
 */
UCLASS()
class UAgAbility_KnockUp : public UAgAbility_HitReaction
{
	GENERATED_BODY()

public:
	UAgAbility_KnockUp();

protected:
	virtual UAnimMontage* GetReactionMontage() const override;
	virtual UAnimMontage* GetFollowUpMontage() const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	UFUNCTION()
	void HandleFalling();

	UFUNCTION()
	void HandleLanded(EMovementMode NewMovementMode);

	UAnimMontage* GetFallMontage() const;

	bool bFalling = false;
	bool bRootMotionIgnored = false;
	TEnumAsByte<ERootMotionMode::Type> SavedRootMotionMode = ERootMotionMode::RootMotionFromMontagesOnly;
};

/** 가드 밀림: pushed back while keeping the guard; attacks during it are guarded too. */
UCLASS()
class UAgAbility_GuardPushback : public UAgAbility_HitReaction
{
	GENERATED_BODY()

public:
	UAgAbility_GuardPushback();

protected:
	virtual UAnimMontage* GetReactionMontage() const override;
};

/** 가드 브레이크: the guard is forced open and the character staggers. */
UCLASS()
class UAgAbility_GuardBreak : public UAgAbility_HitReaction
{
	GENERATED_BODY()

public:
	UAgAbility_GuardBreak();

protected:
	virtual UAnimMontage* GetReactionMontage() const override;
};
