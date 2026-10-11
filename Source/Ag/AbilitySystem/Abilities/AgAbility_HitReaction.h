// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "Animation/AnimEnums.h"
#include "AgAbility_HitReaction.generated.h"

class UAbilityTask_PlayMontageAndWait;

/**
 * 넉업, 넉백, 가드 밀림, 가드 브레이크 (전투 시스템 '피격 모션', '가드 반응'). 피격 이벤트로 시작합니다.
 * 하던 행동이나 보스 패턴을 멈추고 공격자 쪽으로 돌아 모션을 재생하며, 끝날 때까지 아무 행동도 할 수 없습니다.
 * 반응은 블렌드 아웃 시작이 아니라 마지막 모션이 끝날 때 끝납니다. 새 넉업·넉백은 새 반응으로 모션을 처음부터 재생합니다.
 * 바닥에 쓰러진 순간을 표시한 모션(Event.DownGrounded: 넉백, 넉업)은 그때부터 반응이 끝날 때까지 무적입니다.
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

	/** 반응 몽타주가 끝나면 이어서 재생하는 몽타주(넉업: 기상) */
	virtual UAnimMontage* GetFollowUpMontage() const { return nullptr; }

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** Plays a montage of this reaction from StartSection (none: the start); the follow-up plays when it ends. */
	void PlayMontage(UAnimMontage* Montage, FName StartSection = NAME_None);

	/** 모션의 루트 이동을 끕니다. 루트 모션 소스로 옮기는 동안 씁니다(몽타주의 루트 이동이 소스보다 우선합니다). */
	void IgnoreMotionRootMotion();

	/** 꺼 둔 모션의 루트 이동을 되돌립니다. 반응이 끝날 때도 되돌립니다. */
	void RestoreMotionRootMotion();

private:
	/** Plays the follow-up once, if there is one. */
	bool PlayFollowUp();

	UFUNCTION()
	void HandleMontageBlendingOut();

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageCancelled();

	UFUNCTION()
	void HandleGrounded(FGameplayEventData Payload);

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	bool bPlayingFollowUp = false;
	bool bInvincible = false;
	bool bRootMotionIgnored = false;
	TEnumAsByte<ERootMotionMode::Type> SavedRootMotionMode = ERootMotionMode::RootMotionFromMontagesOnly;
};

/**
 * 넉백: 뒤로 크게 밀려나며 쓰러졌다가 일어남 (전투 시스템 '피격 반응'). 플레이어와 보스 모두 씁니다.
 * 넉백 모션 하나에 쓰러짐과 기상이 다 들어 있고, 모션의 루트 이동대로 물러납니다.
 */
UCLASS()
class UAgAbility_KnockBack : public UAgAbility_HitReaction
{
	GENERATED_BODY()

public:
	UAgAbility_KnockBack();

protected:
	virtual UAnimMontage* GetReactionMontage() const override;
};

/**
 * 넉업: 공중으로 떠올랐다가 떨어진 뒤 일어남 (전투 시스템 '넉업').
 * 루트 모션 점프로 중력과 관계없이 높이와 체공 시간의 포물선을 따라 곧게 오르내립니다. 그동안 모션의 루트 이동은 꺼서
 * 떨어진 자리에서 착지합니다. 정점 무렵 상승 자세가 낙하 자세로 바뀌고, 착지하면 낙하 몽타주의 Land 섹션과 기상을 잇습니다.
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

private:
	UFUNCTION()
	void HandleFalling();

	UFUNCTION()
	void HandleLanded(EMovementMode NewMovementMode);

	UAnimMontage* GetFallMontage() const;

	bool bFalling = false;
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
