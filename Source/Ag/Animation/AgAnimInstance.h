// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "AgAnimInstance.generated.h"

class AAgCharacterBase;
class UAnimSequence;
class UAnimSequenceBase;
class UBlendSpace;

UCLASS()
class UAgAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	bool IsTurningInPlace() const;

	float ConsumeTurnYaw();

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets")
	TObjectPtr<UBlendSpace> MoveBlendSpace;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets")
	TObjectPtr<UBlendSpace> GuardMoveBlendSpace;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets")
	TObjectPtr<UAnimSequenceBase> AirborneAnimation;

	UPROPERTY(EditDefaultsOnly, Category = "Assets")
	TArray<TObjectPtr<UAnimSequence>> TurnAnimations;

	UPROPERTY(EditDefaultsOnly, Category = "Assets")
	TArray<TObjectPtr<UAnimSequence>> GuardTurnAnimations;

	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace")
	float MaxTurnAngle = 135.f;

	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace")
	float PlayRateAtMaxAngle = 1.3f;

	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace")
	float PlayRateOnDirectionChange = 1.7f;

	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace")
	float TurnBlendTime = 0.1f;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float GroundSpeed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float MoveDirection = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsInAir = false;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsGuarding = false;

	UPROPERTY(BlueprintReadOnly, Category = "TurnInPlace")
	TObjectPtr<UAnimSequence> TurnAnimation;

	UPROPERTY(BlueprintReadOnly, Category = "TurnInPlace")
	float TurnTime = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "TurnInPlace")
	float TurnWeight = 0.f;

private:
	void UpdateTurnInPlace(float DeltaSeconds);
	void StartTurn(float Offset);
	void AdvanceTurn(float DeltaSeconds);
	bool CanTurn() const;
	const TArray<TObjectPtr<UAnimSequence>>& GetTurnAnimations() const;
	float GetTurnOffset() const;

	UPROPERTY(Transient)
	TObjectPtr<AAgCharacterBase> Character;

	float PendingTurnYaw = 0.f;
	float TurnDirection = 0.f;
	float TurnScale = 1.f;
	bool bTurning = false;
	bool bReachedMaxTurnAngle = false;
};
