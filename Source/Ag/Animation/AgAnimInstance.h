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

public:
	/** Copies the blend spaces, airborne loop and blend times from the character data. */
	void ApplyCharacterData();

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

	/** From the character data. The guard blend's True Blend Time. */
	UPROPERTY(BlueprintReadOnly, Category = "Blend")
	float GuardBlendInTime = 0.f;

	/** From the character data. The guard blend's False Blend Time. */
	UPROPERTY(BlueprintReadOnly, Category = "Blend")
	float GuardBlendOutTime = 0.f;

	/** From the character data. Both blend times of the airborne blend. */
	UPROPERTY(BlueprintReadOnly, Category = "Blend")
	float AirborneBlendTime = 0.f;

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
