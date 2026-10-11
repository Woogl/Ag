// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_Execution.generated.h"

class AAgCharacterBase;
class AAgPlayerCharacter;

/**
 * 처형 (플레이어 사양 '처형'): a finishing move on a groggy boss within the 처형 타겟팅 범위, once per groggy.
 * The boss turns to the player and starts its 처형 피격 montage in the same frame as the execution montage; that montage
 * waits on its first frame until the first slash, so the two motions stay in step frame by frame. By the first slash the
 * player closes in to the boss's front with Motion Warping. The player is invincible until the execution ends. Each
 * strike of the motion (the two slashes and the final blow, one Execution hit each) applies its damage without hitstop.
 * The execution has no recovery (후딜), so no action can cancel it before the motion ends.
 */
UCLASS()
class UAgAbility_Execution : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgAbility_Execution();

	/** 지금 처형할 수 있는 대상: 발동 조건 1, 2를 만족하는 가장 가까운 적('대상 찾기'). 없으면 null입니다. */
	static AAgCharacterBase* FindExecutionTarget(const AAgPlayerCharacter* Player);

protected:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	UFUNCTION()
	void HandleStrike(FGameplayEventData Payload);

	UFUNCTION()
	void HandleMontageFinished();

	UFUNCTION()
	void HandleMontageCancelled();

	TWeakObjectPtr<AAgCharacterBase> Target;

	/** Strikes so far in this execution; the last Execution hit is the final blow. */
	int32 StrikeIndex = 0;
};
