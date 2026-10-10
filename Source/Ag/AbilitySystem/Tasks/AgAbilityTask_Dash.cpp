// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_Dash.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"

namespace
{
	/** A tick that moves less than this share of the expected distance counts as blocked. */
	constexpr float BlockedMoveRatio = 0.3f;

	/** Blocked ticks in a row before the dash stops (one short tick can be a step). */
	constexpr int32 BlockedTicksToStop = 3;

	/** Ticks expected to move less than this aren't judged (hitstop all but stops the dash on purpose). */
	constexpr float MinJudgedMove = 2.f;
}

UAgAbilityTask_Dash::UAgAbilityTask_Dash()
{
	bTickingTask = true;
}

UAgAbilityTask_Dash* UAgAbilityTask_Dash::Dash(UGameplayAbility* OwningAbility, const FVector& Direction, float Speed, float Distance)
{
	UAgAbilityTask_Dash* Task = NewAbilityTask<UAgAbilityTask_Dash>(OwningAbility);
	Task->Direction = Direction.GetSafeNormal2D();
	Task->Speed = Speed;
	Task->Distance = Distance;
	return Task;
}

void UAgAbilityTask_Dash::Activate()
{
	const ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
	Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Movement.IsValid() || Speed <= 0.f || Distance <= 0.f || Direction.IsNearlyZero())
	{
		Finish();
		return;
	}

	StartLocation = LastLocation = Character->GetActorLocation();

	// The dash velocity overrides everything else, including the motion's own root motion, until it is removed.
	TSharedPtr<FRootMotionSource_ConstantForce> Source = MakeShared<FRootMotionSource_ConstantForce>();
	Source->InstanceName = TEXT("AgDash");
	Source->AccumulateMode = ERootMotionAccumulateMode::Override;
	Source->Priority = 500;
	Source->Force = Direction * Speed;
	Source->Duration = -1.f;
	Source->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
	Source->FinishVelocityParams.SetVelocity = FVector::ZeroVector;
	SourceID = Movement->ApplyRootMotionSource(Source);
}

void UAgAbilityTask_Dash::TickTask(float DeltaTime)
{
	const AActor* Avatar = GetAvatarActor();
	if (!Avatar || IsFinished())
	{
		return;
	}

	const FVector Location = Avatar->GetActorLocation();
	if (FVector::Dist2D(StartLocation, Location) >= Distance)
	{
		Finish();
		return;
	}

	// Blocked (a wall): stop where it is. Hitstop slows the owner's time, so those ticks expect almost no movement.
	const float Expected = Speed * DeltaTime;
	if (Expected >= MinJudgedMove)
	{
		BlockedTicks = FVector::Dist2D(LastLocation, Location) < Expected * BlockedMoveRatio ? BlockedTicks + 1 : 0;
	}
	LastLocation = Location;
	if (BlockedTicks >= BlockedTicksToStop)
	{
		Finish();
	}
}

void UAgAbilityTask_Dash::Finish()
{
	RemoveSource();
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnFinished.Broadcast();
	}
	EndTask();
}

void UAgAbilityTask_Dash::RemoveSource()
{
	if (SourceID != 0 && Movement.IsValid())
	{
		Movement->RemoveRootMotionSourceByID(SourceID);
		Movement->Velocity = FVector::ZeroVector;
	}
	SourceID = 0;
}

void UAgAbilityTask_Dash::OnDestroy(bool bInOwnerFinished)
{
	RemoveSource();
	Super::OnDestroy(bInOwnerFinished);
}
