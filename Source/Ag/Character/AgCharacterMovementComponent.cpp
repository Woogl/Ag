// Copyright Woogle. All Rights Reserved.

#include "Character/AgCharacterMovementComponent.h"

#include "Character/AgCharacterBase.h"

float UAgCharacterMovementComponent::GetFallHeight(const FHitResult& LandingHit) const
{
	return FallTopZ - LandingHit.ImpactPoint.Z;
}

void UAgCharacterMovementComponent::PhysicsRotation(float DeltaTime)
{
	if (!CastChecked<AAgCharacterBase>(CharacterOwner)->IsFacingFixed())
	{
		Super::PhysicsRotation(DeltaTime);
	}
}

void UAgCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

	if (IsFalling() && PreviousMovementMode != MOVE_Falling)
	{
		FallTopZ = GetActorFeetLocation().Z;
	}
}

void UAgCharacterMovementComponent::PhysWalking(float DeltaTime, int32 Iterations)
{
	// 발판 끝에서 걸어 나가면 캡슐 아래쪽이 모서리를 타고 내려간 뒤, 엔진이 새 위치에서 바닥을 다시 찾고서 떨어뜨립니다.
	// 그래서 떨어질 때의 발보다 이 이동 전에 딛고 있던 바닥이 높습니다.
	const FFindFloorResult Floor = CurrentFloor;
	Super::PhysWalking(DeltaTime, Iterations);
	if (IsFalling() && Floor.IsWalkableFloor())
	{
		FallTopZ = FMath::Max(FallTopZ, Floor.HitResult.ImpactPoint.Z);
	}
}

void UAgCharacterMovementComponent::PhysFalling(float DeltaTime, int32 Iterations)
{
	// 엔진은 이 함수 안에서 착지를 알리므로, 이동하기 전에 갱신합니다. 오르는 동안 낮은 곳에서 다시 띄워지면
	// 그 지점부터 다시 셉니다.
	const float Z = GetActorFeetLocation().Z;
	if (Velocity.Z > 0.f || Z > FallTopZ)
	{
		FallTopZ = Z;
	}

	Super::PhysFalling(DeltaTime, Iterations);
}
