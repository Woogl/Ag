// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "AgGameplayCue_WeaponEffect.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

/**
 * Plays a looping effect on the target's weapon while the cue is active (붉은 섬광).
 * The effect is data on the cue blueprint; the weapon comes in the cue parameters (TargetAttachComponent).
 */
UCLASS(Abstract)
class AAgGameplayCue_WeaponEffect : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	AAgGameplayCue_WeaponEffect();

	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TObjectPtr<UNiagaraSystem> Effect;

	/** Socket on the weapon to attach to; none attaches at the weapon's origin. */
	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	FName AttachSocket;

private:
	UPROPERTY()
	TObjectPtr<UNiagaraComponent> SpawnedEffect;
};
