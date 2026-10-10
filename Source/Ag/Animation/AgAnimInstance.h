// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "AgAnimInstance.generated.h"

class AAgCharacterBase;
class UAnimSequenceBase;
class UBlendSpace;

/**
 * Parent of ABP_Ag, shared by the player and the boss.
 * Computes every value the anim graph reads; the graph only blends (locomotion, guard locomotion, falling, montage slot, additive slot).
 */
UCLASS()
class UAgAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	/** From the character data. */
	UPROPERTY(BlueprintReadOnly, Category = "Assets")
	TObjectPtr<UBlendSpace> MoveBlendSpace;

	/** From the character data. Empty for a character that never guards. */
	UPROPERTY(BlueprintReadOnly, Category = "Assets")
	TObjectPtr<UBlendSpace> GuardMoveBlendSpace;

	/** From the character data. */
	UPROPERTY(BlueprintReadOnly, Category = "Assets")
	TObjectPtr<UAnimSequenceBase> AirborneAnimation;

	/** Horizontal speed (cm/s). */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float GroundSpeed = 0.f;

	/** Movement direction relative to where the character faces (deg): 0 forward, 90 right, -90 left, +-180 backward. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float MoveDirection = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsInAir = false;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsGuarding = false;

private:
	UPROPERTY(Transient)
	TObjectPtr<AAgCharacterBase> Character;
};
