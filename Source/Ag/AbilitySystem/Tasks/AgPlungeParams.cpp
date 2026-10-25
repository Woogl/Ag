// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgPlungeParams.h"

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
	// 몽타주 루트 모션을 끄지 않으므로, 쓰는 몽타주는 모든 시퀀스의 루트 모션을 끕니다.
	// 루트 모션 시퀀스가 하나라도 있으면 엔진이 몽타주 전체를 루트 모션 몽타주로 다룹니다.
	// 지속 시간이 없으므로 구간이 끝날 때 끝납니다.
	const FVector Velocity(Movement->Velocity.X, Movement->Velocity.Y, -Speed);
	const FVector Direction = Velocity.GetSafeNormal();
	const float Strength = static_cast<float>(Velocity.Size());
	return UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(&Context.Ability, TEXT("AgPlunge"), Direction, Strength, -1.f, false,
		nullptr, ERootMotionFinishVelocityMode::SetVelocity, FVector::ZeroVector, 0.f, false);
}
