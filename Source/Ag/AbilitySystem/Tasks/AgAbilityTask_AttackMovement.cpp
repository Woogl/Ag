// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_AttackMovement.h"

#include "Character/AgCharacterBase.h"
#include "Combat/AgCombatLibrary.h"
#include "MotionWarpingComponent.h"

UAgAbilityTask_AttackMovement::UAgAbilityTask_AttackMovement()
{
	bTickingTask = true;
}

UAgAbilityTask_AttackMovement* UAgAbilityTask_AttackMovement::KeepGap(UGameplayAbility* OwningAbility, AActor* InTarget, float InGap, float InMaxTravel, float InMaxRange)
{
	UAgAbilityTask_AttackMovement* Task = NewAbilityTask<UAgAbilityTask_AttackMovement>(OwningAbility);
	Task->Target = InTarget;
	Task->Gap = InGap;
	Task->MaxTravel = InMaxTravel;
	Task->MaxRange = InMaxRange;
	Task->bKeepGap = true;
	return Task;
}

UAgAbilityTask_AttackMovement* UAgAbilityTask_AttackMovement::FitGap(UGameplayAbility* OwningAbility, AActor* InTarget, float InGap)
{
	UAgAbilityTask_AttackMovement* Task = NewAbilityTask<UAgAbilityTask_AttackMovement>(OwningAbility);
	Task->Target = InTarget;
	Task->Gap = InGap;
	Task->bKeepGap = false;
	return Task;
}

void UAgAbilityTask_AttackMovement::Activate()
{
	Super::Activate();

	AAgCharacterBase* Owner = Cast<AAgCharacterBase>(GetAvatarActor());
	if (!Owner)
	{
		EndTask();
		return;
	}
	Character = Owner;
	Origin = Owner->GetActorLocation();
	if (bKeepGap)
	{
		Owner->SetStopTarget(Target.Get(), Gap);
	}
	bFollowing = true;
	UpdateWarpTarget();
}

void UAgAbilityTask_AttackMovement::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	const AAgCharacterBase* Owner = Character.Get();
	if (!bFollowing || !Owner)
	{
		return;
	}
	if (Owner->IsFacingFixed())
	{
		// 바라보는 방향과 함께 접근 목표도 고정합니다. 마지막 목표는 워핑 구간이 끝날 때까지 쓰입니다.
		bFollowing = false;
		return;
	}
	UpdateWarpTarget();
}

void UAgAbilityTask_AttackMovement::UpdateWarpTarget()
{
	AAgCharacterBase* Owner = Character.Get();
	const AActor* TargetActor = Target.Get();
	const float Distance = UAgCombatLibrary::GetHorizontalDistance(Owner, TargetActor);
	if (!Owner || !TargetActor || (MaxRange >= 0.f && Distance > MaxRange))
	{
		// 범위 밖: 따라가기를 멈추고, 모션은 만든 그대로 마지막 방향으로 계속합니다.
		bFollowing = false;
		if (Owner)
		{
			Owner->GetMotionWarping()->RemoveWarpTarget(AAgCharacterBase::AttackWarpTarget);
		}
		return;
	}

	const FVector Location = Owner->GetActorLocation();
	const FVector Direction = (TargetActor->GetActorLocation() - Location).GetSafeNormal2D();
	FVector Goal = Location;
	if (Distance > Gap || !bKeepGap)
	{
		Goal = TargetActor->GetActorLocation() - Direction * Gap;
		Goal.Z = Location.Z;
	}
	if (MaxTravel >= 0.f)
	{
		const FVector Offset = (Goal - Origin).GetClampedToMaxSize2D(MaxTravel);
		Goal = FVector(Origin.X + Offset.X, Origin.Y + Offset.Y, Location.Z);
	}

	// 방향은 늘 대상을 바라보게 두고, 실제로 돌릴지는 몽타주 워핑 구간의 방향 워프 설정이 정합니다(보스 몽타주는 끔).
	const FRotator Facing = Direction.IsNearlyZero() ? Owner->GetActorRotation() : Direction.Rotation();
	Owner->GetMotionWarping()->AddOrUpdateWarpTargetFromLocationAndRotation(AAgCharacterBase::AttackWarpTarget, Goal, Facing);
}

void UAgAbilityTask_AttackMovement::OnDestroy(bool bInOwnerFinished)
{
	if (AAgCharacterBase* Owner = Character.Get())
	{
		if (bKeepGap)
		{
			Owner->ClearStopTarget();
		}
		Owner->GetMotionWarping()->RemoveWarpTarget(AAgCharacterBase::AttackWarpTarget);
	}

	Super::OnDestroy(bInOwnerFinished);
}
