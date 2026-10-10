// Copyright Woogle. All Rights Reserved.

#include "Animation/AgAnimInstance.h"

#include "AbilitySystemComponent.h"
#include "Character/AgCharacterBase.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "GameFramework/CharacterMovementComponent.h"

void UAgAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Character = Cast<AAgCharacterBase>(TryGetPawnOwner());
	if (const UAgCharacterData* Data = Character ? Character->GetCharacterData() : nullptr)
	{
		MoveBlendSpace = Data->MoveBlendSpace;
		GuardMoveBlendSpace = Data->GuardMoveBlendSpace;
		AirborneAnimation = Data->AirborneAnimation;
	}
}

void UAgAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!Character)
	{
		return;
	}

	const FVector Velocity = Character->GetVelocity();
	GroundSpeed = Velocity.Size2D();
	if (GroundSpeed > UE_KINDA_SMALL_NUMBER)
	{
		// Keep the last direction while standing so the blend doesn't snap.
		const FVector LocalVelocity = Character->GetActorRotation().UnrotateVector(Velocity);
		MoveDirection = FMath::RadiansToDegrees(FMath::Atan2(LocalVelocity.Y, LocalVelocity.X));
	}

	bIsInAir = Character->GetCharacterMovement()->IsFalling();

	const UAbilitySystemComponent* AbilitySystem = Character->GetAbilitySystemComponent();
	bIsGuarding = GuardMoveBlendSpace && AbilitySystem && AbilitySystem->HasMatchingGameplayTag(AgGameplayTags::State_Guarding);
}
