// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_Teleport.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/AgCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "Core/AgGameplayTags.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace AgAbilityTask_TeleportPrivate
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
	 * 대상의 등 뒤(대상 정면의 반대)에서 좌우 MaxAngle 안의, 정후방에 가장 가까운 나타날 수 있는 캡슐 중심 위치입니다.
	 * 대상이 떠 있으면 그 아래 바닥에서 찾습니다.
	 */
	TOptional<FVector> FindAppearLocation(const ACharacter& Character, const ACharacter& Target, float Distance, float MaxAngle)
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
		for (float Angle = AngleStep; Angle <= MaxAngle; Angle += AngleStep)
		{
			Angles.Add(Angle);
			Angles.Add(-Angle);
		}

		const FVector Behind = -Target.GetActorForwardVector().GetSafeNormal2D();
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AgTeleport), false, &Character);
		QueryParams.AddIgnoredActor(&Target);
		for (const float Angle : Angles)
		{
			const FVector Horizontal = TargetLocation + Behind.RotateAngleAxis(Angle, FVector::UpVector) * Distance;
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
} // namespace AgAbilityTask_TeleportPrivate

UAbilityTask* FAgTeleportParams::MakeTask(const FAgTaskContext& Context) const
{
	UAgAbilityTask_Teleport* Task = UAbilityTask::NewAbilityTask<UAgAbilityTask_Teleport>(&Context.Ability);
	Task->Params = *this;
	return Task;
}

void UAgAbilityTask_Teleport::Activate()
{
	EventHandle = AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(Params.Event).AddUObject(this, &ThisClass::HandleEvent);
}

void UAgAbilityTask_Teleport::OnDestroy(bool bInOwnerFinished)
{
	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get(); ASC && EventHandle.IsValid())
	{
		ASC->GenericGameplayEventCallbacks.FindOrAdd(Params.Event).Remove(EventHandle);
	}
	Super::OnDestroy(bInOwnerFinished);
}

void UAgAbilityTask_Teleport::HandleEvent(const FGameplayEventData* Payload)
{
	// 같은 때 맞은 다른 공격(반사한 투사체 등)의 상대에게는 가지 않습니다.
	AAgCharacterBase* Target = Cast<AAgCharacterBase>(const_cast<AActor*>(Payload->Instigator.Get()));
	if (IsFinished() || Payload->ContextHandle.GetAbilityInstance_NotReplicated() != Ability || !Target || Target->IsDead())
	{
		return;
	}
	AAgCharacterBase* Character = CastChecked<AAgCharacterBase>(GetAvatarActor());
	const TOptional<FVector> Location =
		AgAbilityTask_TeleportPrivate::FindAppearLocation(*Character, *Target, Params.Distance, Params.MaxAngle);
	if (!Location)
	{
		return;
	}

	FGameplayCueParameters CueParameters;
	CueParameters.Location = Character->GetActorLocation();
	AbilitySystemComponent->ExecuteGameplayCue(AgGameplayTags::GameplayCue_Teleport, CueParameters);
	Character->TeleportTo(*Location, FRotator(0.f, (Target->GetActorLocation() - *Location).Rotation().Yaw, 0.f), false, true);
	CueParameters.Location = Character->GetActorLocation();
	AbilitySystemComponent->ExecuteGameplayCue(AgGameplayTags::GameplayCue_Teleport, CueParameters);
	CastChecked<UAgGameplayAbility>(Ability)->SetActionTarget(Target);
	EndTask();

	FGameplayEventData Teleported;
	Teleported.EventTag = AgGameplayTags::Event_Teleported;
	Teleported.Instigator = Target;
	Teleported.Target = Character;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Character, AgGameplayTags::Event_Teleported, Teleported);
}
