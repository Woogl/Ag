// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AgCharacterMovementComponent.generated.h"

UCLASS()
class UAgCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	/** 공중에서 마지막으로 발이 가장 높았던 지점(걸어 나가 떨어지면 딛고 있던 바닥)부터 착지한 바닥까지의 높이 */
	float GetFallHeight(const FHitResult& LandingHit) const;

protected:
	virtual void PhysicsRotation(float DeltaTime) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	virtual void PhysWalking(float DeltaTime, int32 Iterations) override;
	virtual void PhysFalling(float DeltaTime, int32 Iterations) override;

private:
	float FallTopZ = 0.f;
};
