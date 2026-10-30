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

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets")
	TObjectPtr<UBlendSpace> MoveBlendSpace;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets")
	TObjectPtr<UBlendSpace> GuardMoveBlendSpace;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets")
	TObjectPtr<UAnimSequenceBase> AirborneAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets")
	TObjectPtr<UAnimSequenceBase> GroggyAnimation;

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

	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace")
	float RootYawOffsetSmoothingTime = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace")
	float MaxUpperBodyTwist = 60.f;

	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace")
	float UpperBodyTwistSmoothingTime = 0.15f;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float GroundSpeed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float MoveDirection = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsInAir = false;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsGuarding = false;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsGroggy = false;

	UPROPERTY(BlueprintReadOnly, Category = "TurnInPlace")
	TObjectPtr<UAnimSequence> TurnAnimation;

	UPROPERTY(BlueprintReadOnly, Category = "TurnInPlace")
	float TurnTime = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "TurnInPlace")
	float TurnWeight = 0.f;

	// 몸과 액터의 Yaw 차이로, 애님 그래프의 Rotate Root Bone이 적용합니다. 엔진 Offset Root Bone은 실험 기능이고 이 차이를
	// 노드가 가지고 있어, 노드 함수를 연결하지 않으면 C++가 회전 모션을 시작할지 판단할 수 없으므로 쓰지 않습니다.
	UPROPERTY(BlueprintReadOnly, Category = "TurnInPlace")
	float RootYawOffset = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "TurnInPlace")
	FRotator UpperBodyTwist = FRotator::ZeroRotator;

private:
	void UpdateTurnInPlace(float DeltaSeconds);
	void StartTurn(float Offset);
	void AdvanceTurn(float DeltaSeconds);
	bool CanTurn() const;
	const TArray<TObjectPtr<UAnimSequence>>& GetTurnAnimations() const;
	/** 컨트롤러를 읽으므로 CanTurn()이 참일 때만 부릅니다. */
	float GetTurnOffset() const;

	UPROPERTY(Transient)
	TObjectPtr<AAgCharacterBase> Character;

	float LastActorYaw = 0.f;
	float TurnDirection = 0.f;
	bool bTurning = false;
	bool bReachedMaxTurnAngle = false;
};
