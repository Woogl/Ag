// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_Execution.generated.h"

class AAgBossCharacter;
class AAgPlayerCharacter;

/**
 * 처형 (플레이어 사양 '처형'): a finishing move on a groggy boss within the 처형 타겟팅 범위, once per groggy.
 * The boss turns to the player and plays its 처형 피격 motion while the player closes in to the boss's front with
 * Motion Warping. The player is invincible until the execution ends or is cancelled. The final blow applies the damage
 * once, without hitstop, and the recovery (후딜) starts after it.
 */
UCLASS()
class UAgAbility_Execution : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgAbility_Execution();

	/** The boss the player can execute now (발동 조건 1 and 2), or null. */
	static AAgBossCharacter* FindExecutableBoss(const AAgPlayerCharacter* Player);

protected:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	void SetExecuting(bool bNewExecuting);

	UFUNCTION()
	void HandleFinalBlow(FGameplayEventData Payload);

	UFUNCTION()
	void HandleMontageFinished();

	UFUNCTION()
	void HandleMontageCancelled();

	TWeakObjectPtr<AAgBossCharacter> Target;
	bool bExecuting = false;
	bool bInvincible = false;
	bool bBlowLanded = false;
};
