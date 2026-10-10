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

	// 회전: follows smoothly up to the maximum turning speed, so a dash or a leap overhead doesn't jerk the view.
	FRotator Current = PlayerController->GetControlRotation();
	Current.Pitch = FRotator::NormalizeAxis(Current.Pitch);
	PlayerController->SetControlRotation(FMath::RInterpConstantTo(Current, Goal, DeltaTime, CameraData->LockOnRotationSpeed));
}
