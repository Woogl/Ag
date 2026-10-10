// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AgBossPatternAbility.h"
#include "Animation/AnimEnums.h"
#include "AgBossPattern_Dash.generated.h"

/**
 * A3 돌진 찌르기 (보스 사양 '돌진 규칙', '돌진과 모션'): tracks the player until the dash starts, then dashes straight
 * past the player at the dash speed without turning and through the player. The dash is the attack window.
 * The thrust pose holds until the dash ends, then the motion recovers without moving further. The motion's own root
 * motion is ignored from the dash on, since the character movement drops other root motion while it plays.
 */
UCLASS()
class UAgBossPattern_Dash : public UAgBossPatternAbility
{
	GENERATED_BODY()

protected:
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual bool UsesSharedMovement() const override { return false; }
	virtual void OnPatternStarted() override;
	virtual void GetUnguardableHitStarts(TArray<float>& OutTimes) const override;

private:
	/** Turns the dash state on or off: no turning, no blocking the player, the attack window open. */
	void SetDashing(bool bNewDashing);

	UFUNCTION()
	void HandleDashStart(FGameplayEventData Payload);

	UFUNCTION()
	void HandleDashHold(FGameplayEventData Payload);

	UFUNCTION()
	void HandleDashFinished();

	bool bDashing = false;
	bool bHolding = false;
	bool bRootMotionIgnored = false;
	TEnumAsByte<ERootMotionMode::Type> SavedRootMotionMode = ERootMotionMode::RootMotionFromMontagesOnly;
};
