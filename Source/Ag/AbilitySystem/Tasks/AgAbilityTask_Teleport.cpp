// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_Teleport.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Character/AgCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "Core/AgGameplayTags.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	constexpr float AngleStep = 15.f;
	constexpr float MaxTargetFloorDepth = 1000.f;

	/**
	 * 캡슐 중심이 Location일 때 중심에서 내린 선이 캡슐 아래 Depth 안에서 닿는 걸을 수 있는 바닥 높이입니다.
	 * 캡슐로 쓸어 내리면 중심이 단 밖으로 나가도 가장자리에 걸쳐 바닥으로 잡히므로 선으로 봅니다.
	 */
	TOptional<double> FindFloorZ(const UCharacterMovementComponent& Movement, const FVector& Location, float Depth)
	{
		const UCapsuleComponent* Capsule = Movement.GetCharacterOwner()->GetCapsuleComponent();
		const FVector End = Location - FVector(0.f, 0.f, Capsule->GetScaledCapsuleHalfHeight() + Depth);
		const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AgTeleportFloor), false, Movement.GetOwner());
		FHitResult Hit;
		if (!Movement.GetWorld()->LineTraceSingleByChannel(Hit, Location, End, Capsule->GetCollisionObjectType(), QueryParams,
				FCollisionResponseParams(Capsule->GetCollisionResponseToChannels())) ||
			!Movement.IsWalkable(Hit))
		{
			return {};
		}
		return Hit.ImpactPoint.Z;
	}

	/**
	 * 중심 아래 바닥이 Floor인 곳에 선 캡슐 중심 위치입니다. 비탈과 계단에서는 아래 반구가 중심 아래보다 높은 곳에 먼저 닿으므로,
	 * 이동 컴포넌트처럼 캡슐로 쓸어 내려 높이를 정합니다.
	 */
	TOptional<FVector> FindStandLocation(const UCharacterMovementComponent& Movement, const FVector& Floor)
	{
		const UCapsuleComponent* Capsule = Movement.GetCharacterOwner()->GetCapsuleComponent();
		const FVector Start = Floor + FVector(0.f, 0.f, Movement.MaxStepHeight + Capsule->GetScaledCapsuleHalfHeight());
		FFindFloorResult FloorResult;
		Movement.ComputeFloorDist(Start, 0.f, 2.f * Movement.MaxStepHeight, FloorResult, Capsule->GetScaledCapsuleRadius());
		if (!FloorResult.IsWalkableFloor())
		{
			return {};
		}
		return Start - FVector(0.f, 0.f, FloorResult.FloorDist - UCharacterMovementComponent::MIN_FLOOR_DIST);
	}

	/**
	 * 대상 둘레에서 순간이동하는 캐릭터의 반대편(캐릭터에서 대상 너머)에 가장 가까운, 나타날 수 있는 캡슐 중심 위치입니다.
	 * 대상이 떠 있으면 그 아래 바닥에서 찾습니다.
	 */
	TOptional<FVector> FindAppearLocation(const ACharacter& Character, const ACharacter& Target, float Distance)
	{
		const UCharacterMovementComponent* Movement = Character.GetCharacterMovement();
		const UCharacterMovementComponent* TargetMovement = Target.GetCharacterMovement();
		UWorld* World = Character.GetWorld();
		if (!Movement || !TargetMovement || !World)
		{
			return {};
		}
		const float HalfHeight = Character.GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		const float StepHeight = Movement->MaxStepHeight;
		const FVector TargetLocation = Target.GetActorLocation();
		const TOptional<double> TargetFloorZ = TargetMovement->IsFalling()
		                                           ? FindFloorZ(*TargetMovement, TargetLocation, MaxTargetFloorDepth)
		                                           : TOptional<double>(TargetLocation.Z - Target.GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		if (!TargetFloorZ)
		{
			return {};
		}

		TArray<float, TInlineAllocator<25>> Angles = {0.f};
		for (float Angle = AngleStep; Angle < 180.f; Angle += AngleStep)
		{
			Angles.Add(Angle);
			Angles.Add(-Angle);
		}
		Angles.Add(180.f);

		// 두 캐릭터가 겹쳐 반대편 방향이 없으면 캐릭터가 바라보는 쪽 너머로 갑니다.
		FVector Beyond = (TargetLocation - Character.GetActorLocation()).GetSafeNormal2D();
		if (Beyond.IsZero())
		{
			Beyond = Character.GetActorForwardVector().GetSafeNormal2D();
		}
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AgTeleport), false, &Character);
		QueryParams.AddIgnoredActor(&Target);
		for (const float Angle : Angles)
		{
			const FVector Horizontal = TargetLocation + Beyond.RotateAngleAxis(Angle, FVector::UpVector) * Distance;
			// 계단 높이만큼 높은 바닥도 찾도록 그만큼 위에서 내려 봅니다.
			const TOptional<double> FloorZ =
				FindFloorZ(*Movement, FVector(Horizontal.X, Horizontal.Y, *TargetFloorZ + StepHeight + HalfHeight), 2.f * StepHeight);
			const TOptional<FVector> Location = FloorZ ? FindStandLocation(*Movement, FVector(Horizontal.X, Horizontal.Y, *FloorZ)) : NullOpt;
			if (!Location)
			{
				continue;
			}
			const FRotator Facing(0.f, (TargetLocation - *Location).Rotation().Yaw, 0.f);
			if (World->EncroachingBlockingGeometry(&Character, *Location, Facing) ||
				World->LineTraceTestByObjectType(TargetLocation, *Location, FCollisionObjectQueryParams(ECC_WorldStatic), QueryParams))
			{
				continue;
			}
			return Location;
		}
		return {};
	}
} // namespace

UAbilityTask* FAgTeleportParams::MakeTask(const FAgTaskContext& Context) const
{
	UAgAbilityTask_Teleport* Task = UAbilityTask::NewAbilityTask<UAgAbilityTask_Teleport>(&Context.Ability);
	Task->Params = *this;
	return Task;
}

void UAgAbilityTask_Teleport::Activate()
{
	AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor());
	const UAgGameplayAbility* AgAbility = CastChecked<UAgGameplayAbility>(Ability);
	if (!Character)
	{
		EndTask();
		return;
	}

	const ACharacter* Target = Cast<ACharacter>(AgAbility->GetActionTarget());
	const TOptional<FVector> Location = Target ? FindAppearLocation(*Character, *Target, Params.Distance) : NullOpt;
	if (Location)
	{
		Character->TeleportTo(*Location, FRotator(0.f, (Target->GetActorLocation() - *Location).Rotation().Yaw, 0.f), false, true);
	}

	FGameplayCueParameters CueParameters;
	CueParameters.Location = Character->GetActorLocation();
	AbilitySystemComponent->ExecuteGameplayCue(AgGameplayTags::GameplayCue_Teleport, CueParameters);
	EndTask();
}
