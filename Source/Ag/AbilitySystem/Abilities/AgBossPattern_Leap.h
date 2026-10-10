// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AgBossPatternAbility.h"
#include "AgBossPattern_Leap.generated.h"

/**
 * A4 도약 내려찍기 (보스 사양 'A4'): leaps on a ballistic arc under the engine's gravity so it lands after the air
 * time, short of where the player was at the leap. Landing plays the End section and hits everything within the
 * landing radius once.
 */
UCLASS()
class UAgBossPattern_Leap : public UAgBossPatternAbility
{
	GENERATED_BODY()

protected:
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual bool UsesSharedMovement() const override { return false; }
	virtual void OnPatternStarted() override;

private:
	void SetRotationLocked(bool bLocked);

	UFUNCTION()
	void HandleParryFlashTime();

	UFUNCTION()
	void HandleRotationStop();

	UFUNCTION()
	void HandleLanded(EMovementMode NewMovementMode);

	bool bRotationLocked = false;
	bool bLanded = false;
};
