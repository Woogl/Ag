// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgBossPatternAbility.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAgBossData;
struct FAgBossPattern;

/**
 * Shared boss pattern (A1, A2, A5): plays the pattern montage at the pattern play rate with its attack data,
 * keeps the stop distance (보스 사양 '이동') and, for A1 and A5, closes in during the first hit's windup.
 * Each granted spec carries its pattern ID as a dynamic tag. The special patterns derive from this class.
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

	/** Pattern motion play rate: 모션 재생 속도 (and the phase 2 change later). */
	float GetPatternPlayRate() const;

	const FAgBossPattern* GetPattern() const;

private:
	/** How far the montage's own root motion moves forward before the first attack window. */
	static float GetForwardTravelBeforeFirstWindow(const UAnimMontage* Montage);

	UFUNCTION()
	void HandleMontageFinished();

	UFUNCTION()
	void HandleMontageCancelled();
};
