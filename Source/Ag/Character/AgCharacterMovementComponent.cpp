// Copyright Woogle. All Rights Reserved.

#include "Character/AgCharacterMovementComponent.h"

#include "Animation/AgAnimInstance.h"
#include "Character/AgCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"

void UAgCharacterMovementComponent::PhysicsRotation(float DeltaTime)
{
	const AAgCharacterBase* Character = Cast<AAgCharacterBase>(CharacterOwner);
	if (!Character || Character->IsDead() || Character->IsFacingFixed())
	{
		return;
	}

	UAgAnimInstance* AnimInstance = Cast<UAgAnimInstance>(Character->GetMesh()->GetAnimInstance());
	if (!AnimInstance || !AnimInstance->IsTurningInPlace())
	{
		Super::PhysicsRotation(DeltaTime);
		return;
	}

	const float TurnYaw = AnimInstance->ConsumeTurnYaw();
	if (TurnYaw != 0.f)
	{
		FRotator Rotation = UpdatedComponent->GetComponentRotation();
		Rotation.Yaw += TurnYaw;
		MoveUpdatedComponent(FVector::ZeroVector, Rotation, false);
	}
}
