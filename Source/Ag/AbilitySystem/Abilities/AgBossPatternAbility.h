// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgBossPatternAbility.generated.h"

class AAgBossCharacter;
class UAnimMontage;
struct FAgBossPattern;

/**
 * Shared boss pattern (A1, A2, A5): plays the pattern montage at the pattern play rate with its attack data,
 * keeps the stop distance (보스 사양 '이동') and, for A1 and A5, closes in during the first hit's windup.
 * Unguardable hits show the 붉은 섬광 before their attack window, and patterns with a cooldown start it on use.
 * Each granted spec carries its pattern ID as a dynamic tag. The special patterns (A3, A4, A6, B1) derive from this class.
 */
UCLASS()
class UAgBossPatternAbility : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgBossPatternAbility();

	/** The pattern ID a granted spec runs. */
	static FGameplayTag GetPatternTag(const FGameplayAbilitySpec& Spec);

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** 쿨다운 per pattern: the pattern's cooldown tag and time from Boss.json, counted from the pattern start. */
	virtual bool CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

	/** Pattern motion play rate: 모션 재생 속도 times the phase 2 패턴 진행 속도. */
	virtual float GetPatternPlayRate() const;

	const FAgBossPattern* GetPattern() const;

	AAgBossCharacter* GetBoss() const;

	UAnimMontage* GetMontage() const { return Montage; }

	/** The shared movement rules (stop distance, first-hit approach). A3 and A4 move by their own rules. */
	virtual bool UsesSharedMovement() const { return true; }

	/** Called once the pattern montage plays; special patterns start their own movement here. */
	virtual void OnPatternStarted() {}

	/** Montage times at which unguardable hits start (for the 붉은 섬광). By default, their attack windows. */
	virtual void GetUnguardableHitStarts(TArray<float>& OutTimes) const;

	/** Montage times at which parryable hits start (for the 예고 섬광). By default, their attack windows. */
	virtual void GetParryableHitStarts(TArray<float>& OutTimes) const;

	/** Montage time of the first gameplay event notify with EventTag, or a negative value. */
	float FindEventNotifyTime(const FGameplayTag& EventTag) const;

	/** Montage times of every gameplay event notify with EventTag, in order. */
	void FindEventNotifyTimes(const FGameplayTag& EventTag, TArray<float>& OutTimes) const;

	/** 몽타주의 판정 구간이 아닌 타격(A4 착지)의 무기 섬광. 붉은 섬광은 가드 불가, 예고 섬광은 패리 가능한 타격에 씁니다. */
	void SetUnblockableTelegraph(bool bOn);
	void SetParryTelegraph(bool bOn);

private:
	/** How far the montage's own root motion moves forward before the first attack window. */
	static float GetForwardTravelBeforeFirstWindow(const UAnimMontage* Montage);

	/** Shows a telegraph from Lead (montage time) before each hit start until the hit starts. */
	void ScheduleTelegraphs(const TArray<float>& HitStarts, float Lead, bool bParryTelegraph);

	/** Turns a weapon telegraph cue on or off; bActive tracks whether it is on. */
	void SetWeaponTelegraph(const FGameplayTag& Cue, bool& bActive, bool bOn);

	UFUNCTION()
	void HandleUnblockableTelegraphStart();

	UFUNCTION()
	void HandleUnblockableTelegraphEnd();

	UFUNCTION()
	void HandleParryTelegraphStart();

	UFUNCTION()
	void HandleParryTelegraphEnd();

	UFUNCTION()
	void HandleMontageFinished();

	UFUNCTION()
	void HandleMontageCancelled();

	UPROPERTY()
	TObjectPtr<UAnimMontage> Montage;

	bool bUnblockableTelegraphOn = false;
	bool bParryTelegraphOn = false;
};
