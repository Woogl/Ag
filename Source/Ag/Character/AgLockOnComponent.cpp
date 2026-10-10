// Copyright Woogle. All Rights Reserved.

#include "Character/AgLockOnComponent.h"

#include "Character/AgBossCharacter.h"
#include "Character/AgPlayerCharacter.h"
#include "Combat/AgCombatLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/AgSettings.h"
#include "Data/AgCameraData.h"
#include "Data/AgCharacterData.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/**
	 * Moves Angle toward Target on a critically damped spring (it eases in and out), never faster than MaxSpeed. Long
	 * frames are split, because the spring is only accurate for steps under half the smoothing time.
	 */
	float SpringToward(float Angle, float& Rate, float Target, float DeltaTime, float SmoothingTime, float MaxSpeed)
	{
		const int32 Steps = SmoothingTime > 0.f ? FMath::Clamp(FMath::CeilToInt(DeltaTime / (0.5f * SmoothingTime)), 1, 8) : 1;
		const float StepTime = DeltaTime / Steps;
		for (int32 Step = 0; Step < Steps; ++Step)
		{
			const float Start = Angle;
			FMath::CriticallyDampedSmoothing(Angle, Rate, Target, 0.f, StepTime, SmoothingTime);
			Angle = Start + FMath::Clamp(Angle - Start, -MaxSpeed * StepTime, MaxSpeed * StepTime);
			Rate = FMath::Clamp(Rate, -MaxSpeed, MaxSpeed);
		}
		return Angle;
	}
}

UAgLockOnComponent::UAgLockOnComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UAgLockOnComponent::ToggleLockOn()
{
	if (IsLockedOn())
	{
		ReleaseLockOn();
		return;
	}

	Target = FindTarget();
	if (Target.IsValid())
	{
		PitchRate = 0.f;
		YawRate = 0.f;
		SetComponentTickEnabled(true);
		OnLockOnChanged.Broadcast();
	}
}

void UAgLockOnComponent::ReleaseLockOn()
{
	// The free camera carries on from the direction the camera faces now.
	if (Target.IsValid())
	{
		Target.Reset();
		SetComponentTickEnabled(false);
		OnLockOnChanged.Broadcast();
	}
}

AAgBossCharacter* UAgLockOnComponent::FindTarget() const
{
	const AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(GetOwner());
	const UAgPlayerData* Data = Player ? Cast<UAgPlayerData>(Player->GetCharacterData()) : nullptr;
	const APlayerController* PlayerController = Player ? Cast<APlayerController>(Player->GetController()) : nullptr;
	AAgBossCharacter* Boss = Data ? Player->FindBoss(Data->LockOnRange) : nullptr;
	if (!Boss || !PlayerController)
	{
		return nullptr;
	}

	// Only a boss on screen can be locked on.
	FVector2D ScreenPosition;
	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;
	PlayerController->GetViewportSize(ViewportWidth, ViewportHeight);
	const bool bOnScreen = PlayerController->ProjectWorldLocationToScreen(Boss->GetActorLocation(), ScreenPosition)
		&& ScreenPosition.X >= 0.f && ScreenPosition.Y >= 0.f && ScreenPosition.X <= ViewportWidth && ScreenPosition.Y <= ViewportHeight;
	return bOnScreen ? Boss : nullptr;
}

FVector UAgLockOnComponent::GetLockOnPoint() const
{
	const AAgBossCharacter* Boss = Target.Get();
	const UAgBossData* Data = Boss ? Cast<UAgBossData>(Boss->GetCharacterData()) : nullptr;
	if (Data && !Data->LockOnBone.IsNone() && Boss->GetMesh()->GetBoneIndex(Data->LockOnBone) != INDEX_NONE)
	{
		return Boss->GetMesh()->GetBoneLocation(Data->LockOnBone);
	}
	return Boss ? Boss->GetActorLocation() : FVector::ZeroVector;
}

void UAgLockOnComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 락온 해제 조건: the target died or went beyond the release range. (The key is handled in ToggleLockOn.)
	const AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(GetOwner());
	const UAgPlayerData* Data = Player ? Cast<UAgPlayerData>(Player->GetCharacterData()) : nullptr;
	const AAgBossCharacter* Boss = Target.Get();
	if (!Boss || !Data || Boss->IsDead() || UAgCombatLibrary::GetHorizontalDistance(Player, Boss) > Data->LockOnReleaseRange)
	{
		ReleaseLockOn();
		return;
	}

	// The camera isn't part of the character's motion, so hitstop (the owner's time dilation) doesn't pause it.
	UpdateCamera(GetWorld()->GetDeltaSeconds());
}

void UAgLockOnComponent::UpdateCamera(float DeltaTime)
{
	const AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(GetOwner());
	APlayerController* PlayerController = Player ? Cast<APlayerController>(Player->GetController()) : nullptr;
	const UAgCameraData* CameraData = UAgSettings::Get()->GetCameraData();
	if (!PlayerController || !CameraData)
	{
		return;
	}

	// 구도: from the camera pivot toward the lock-on point, pitch limited, then tilted further down.
	const FVector Pivot = Player->GetActorLocation() + FVector(0.f, 0.f, CameraData->PivotHeight);
	FRotator Goal = (GetLockOnPoint() - Pivot).Rotation();
	Goal.Pitch = FMath::Clamp(FRotator::NormalizeAxis(Goal.Pitch), CameraData->LockOnPitchMin, CameraData->LockOnPitchMax) - CameraData->LockOnLookDown;
	Goal.Roll = 0.f;

	// 회전: eases in and out toward the goal and never turns faster than the maximum speed, so locking on, a dash or a
	// leap overhead doesn't jerk the view. Yaw takes the short way round.
	const FRotator Current = PlayerController->GetControlRotation();
	const float CurrentPitch = static_cast<float>(FRotator::NormalizeAxis(Current.Pitch));
	const float CurrentYaw = static_cast<float>(Current.Yaw);
	const float GoalYaw = CurrentYaw + static_cast<float>(FMath::FindDeltaAngleDegrees(Current.Yaw, Goal.Yaw));
	const float Pitch = SpringToward(CurrentPitch, PitchRate, static_cast<float>(Goal.Pitch), DeltaTime, CameraData->LockOnSmoothingTime, CameraData->LockOnRotationSpeed);
	const float Yaw = SpringToward(CurrentYaw, YawRate, GoalYaw, DeltaTime, CameraData->LockOnSmoothingTime, CameraData->LockOnRotationSpeed);
	PlayerController->SetControlRotation(FRotator(Pitch, Yaw, 0.f));
}
