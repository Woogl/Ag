// Copyright Woogle. All Rights Reserved.

#include "Animation/AgAnimInstance.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/BlendSpace.h"
#include "Character/AgCharacterBase.h"
#include "Core/AgGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "KismetAnimationLibrary.h"

namespace
{
	float GetRootYaw(const UAnimSequence& Animation, float Time)
	{
		const FTransform Root = Animation.ExtractRootTrackTransform(FAnimExtractContext(static_cast<double>(Time)), nullptr);
		return static_cast<float>(Root.Rotator().Yaw);
	}

	float GetTurnAngle(const UAnimSequence& Animation)
	{
		return FMath::UnwindDegrees(GetRootYaw(Animation, Animation.GetPlayLength()) - GetRootYaw(Animation, 0.f));
	}

	float GetSmallestTurnAngle(const TArray<TObjectPtr<UAnimSequence>>& Animations)
	{
		float Smallest = TNumericLimits<float>::Max();
		for (const UAnimSequence* Animation : Animations)
		{
			if (Animation)
			{
				Smallest = FMath::Min(Smallest, FMath::Abs(GetTurnAngle(*Animation)));
			}
		}
		return Smallest;
	}
} // namespace

void UAgAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Character = Cast<AAgCharacterBase>(TryGetPawnOwner());
	if (Character)
	{
		LastActorYaw = static_cast<float>(Character->GetActorRotation().Yaw);
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
		// 서 있을 때는 마지막 방향을 유지해 블렌드가 튀지 않게 합니다.
		MoveDirection = UKismetAnimationLibrary::CalculateDirection(Velocity, Character->GetActorRotation());
	}

	bIsInAir = Character->GetCharacterMovement()->IsFalling();

	const UAbilitySystemComponent* AbilitySystem = Character->GetAbilitySystemComponent();
	bIsGuarding = GuardMoveBlendSpace && AbilitySystem && AbilitySystem->HasMatchingGameplayTag(AgGameplayTags::State_Guarding);
	bIsGroggy = GroggyAnimation && AbilitySystem && AbilitySystem->HasMatchingGameplayTag(AgGameplayTags::State_Groggy);

	UpdateTurnInPlace(DeltaSeconds);
}

void UAgAnimInstance::UpdateTurnInPlace(float DeltaSeconds)
{
	const float ActorYaw = static_cast<float>(Character->GetActorRotation().Yaw);
	const bool bCanTurn = CanTurn();
	if (bCanTurn)
	{
		RootYawOffset = FMath::UnwindDegrees(RootYawOffset - (ActorYaw - LastActorYaw));
		if (bTurning)
		{
			AdvanceTurn(DeltaSeconds);
		}

		// 가장 작은 모션보다 작은 차이는 걸음을 크게 줄여야 해 다리가 움찔거리므로, 발을 붙인 채 몸을 그대로 둡니다.
		const float Offset = GetTurnOffset();
		if (!bTurning && FMath::Abs(Offset) > GetSmallestTurnAngle(GetTurnAnimations()))
		{
			StartTurn(Offset);
		}
	}
	else
	{
		// 멈춘 모션과 시간은 그대로 두어, 비율이 0으로 내려가는 동안 그 자세로 빠집니다.
		bTurning = false;
		FMath::ExponentialSmoothingApprox(RootYawOffset, 0.f, DeltaSeconds, RootYawOffsetSmoothingTime);
	}
	LastActorYaw = ActorYaw;

	if (FMath::Abs(RootYawOffset) > MaxTurnAngle)
	{
		RootYawOffset = FMath::Sign(RootYawOffset) * MaxTurnAngle;
		bReachedMaxTurnAngle |= bTurning;
	}

	// 락온 대상을 바꾸면 목표가 한 번에 바뀌므로, 상체가 한 프레임에 돌지 않게 따라가게 합니다.
	const float TwistGoal = bCanTurn ? FMath::Clamp(GetTurnOffset(), -MaxUpperBodyTwist, MaxUpperBodyTwist) : 0.f;
	FMath::ExponentialSmoothingApprox(UpperBodyTwist.Yaw, static_cast<double>(TwistGoal), DeltaSeconds, UpperBodyTwistSmoothingTime);

	TurnWeight = FMath::FInterpConstantTo(TurnWeight, bTurning ? 1.f : 0.f, DeltaSeconds, TurnBlendTime > 0.f ? 1.f / TurnBlendTime : 0.f);
}

void UAgAnimInstance::StartTurn(float Offset)
{
	UAnimSequence* Picked = nullptr;
	float PickedAngle = 0.f;
	for (UAnimSequence* Animation : GetTurnAnimations())
	{
		const float Angle = Animation ? GetTurnAngle(*Animation) : 0.f;
		if (Angle * Offset <= 0.f)
		{
			continue;
		}

		if (!Picked || FMath::Abs(Offset - Angle) < FMath::Abs(Offset - PickedAngle))
		{
			Picked = Animation;
			PickedAngle = Angle;
		}
	}

	if (!Picked)
	{
		return;
	}

	TurnAnimation = Picked;
	TurnTime = 0.f;
	TurnDirection = FMath::Sign(Offset);
	bTurning = true;
	bReachedMaxTurnAngle = false;
}

void UAgAnimInstance::AdvanceTurn(float DeltaSeconds)
{
	const bool bDirectionChanged = GetTurnOffset() * TurnDirection < 0.f;
	const float PlayRate =
		FMath::Max(bReachedMaxTurnAngle ? PlayRateAtMaxAngle : 1.f, bDirectionChanged ? PlayRateOnDirectionChange : 1.f);

	const float Length = TurnAnimation->GetPlayLength();
	const float NewTime = FMath::Min(TurnTime + DeltaSeconds * PlayRate, Length);
	// 회전량을 늘리거나 줄이면 몸이 도는 만큼 걸음이 따라가지 못해 발이 미끄러지므로, 모션의 회전량을 그대로 씁니다.
	RootYawOffset += FMath::UnwindDegrees(GetRootYaw(*TurnAnimation, NewTime) - GetRootYaw(*TurnAnimation, TurnTime));
	TurnTime = NewTime;
	bTurning = TurnTime < Length;
}

bool UAgAnimInstance::CanTurn() const
{
	if (!Character || !Character->GetController() || GetTurnAnimations().IsEmpty() || Character->IsDead() || Character->IsFacingFixed())
	{
		return false;
	}

	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	return Movement->bUseControllerDesiredRotation && Movement->IsMovingOnGround() && Movement->Velocity.IsNearlyZero();
}

const TArray<TObjectPtr<UAnimSequence>>& UAgAnimInstance::GetTurnAnimations() const
{
	return bIsGuarding ? GuardTurnAnimations : TurnAnimations;
}

float UAgAnimInstance::GetTurnOffset() const
{
	// 락온 카메라는 대상을 부드럽게 따라가므로, 락온 중에는 카메라가 아니라 대상의 실제 방향으로 잽니다.
	const AActor* Target = Character->GetLockOnTarget();
	const double DesiredYaw = Target ? (Target->GetActorLocation() - Character->GetActorLocation()).Rotation().Yaw
	                                 : Character->GetController()->GetDesiredRotation().Yaw;
	return static_cast<float>(FRotator::NormalizeAxis(DesiredYaw - Character->GetActorRotation().Yaw - RootYawOffset));
}
