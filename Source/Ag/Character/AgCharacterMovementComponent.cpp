// Copyright Woogle. All Rights Reserved.

#include "Character/AgCharacterMovementComponent.h"

#include "Character/AgCharacterBase.h"

void UAgCharacterMovementComponent::PhysicsRotation(float DeltaTime)
{
	const AAgCharacterBase* Character = Cast<AAgCharacterBase>(CharacterOwner);
	if (!Character || Character->IsDead() || Character->IsFacingFixed())
	{
		return;
	}

	Super::PhysicsRotation(DeltaTime);
}
