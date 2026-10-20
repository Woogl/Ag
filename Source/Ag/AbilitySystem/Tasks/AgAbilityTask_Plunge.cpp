// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_Plunge.h"

#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"

UAbilityTask* FAgPlungeParams::MakeTask(const FAgTaskContext& Context) const
{
	const ACharacter* Character = Cast<ACharacter>(Context.Ability.GetAvatarActorFromActorInfo());
	const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Movement || Speed <= 0.f)
	{
		return nullptr;
	}
	// 지속 시간이 없으므로 구간이 끝날 때 끝납니다.
	const FVector Velocity(Movement->Velocity.X, Movement->Velocity.Y, -Speed);
	return UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(&Context.Ability, TEXT("AgPlunge"), Velocity, static_cast<float>(Velocity.Size()), -1.f, false,
		nullptr, ERootMotionFinishVelocityMode::SetVelocity, FVector::ZeroVector, 0.f, false);
}
