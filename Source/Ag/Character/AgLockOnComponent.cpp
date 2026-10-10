// Copyright Woogle. All Rights Reserved.

#include "Character/AgLockOnComponent.h"

#include "Character/AgBossCharacter.h"
#include "Character/AgPlayerCharacter.h"
#include "Combat/AgCombatLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/AgCharacterData.h"
#include "GameFramework/PlayerController.h"

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

void UAgLockOnComponent::SetTarget(AAgBossCharacter* NewTarget)
{
	// Releasing leaves the camera as it is: the free camera carries on from the direction it faces now.
	if (Target.Get() != NewTarget)
	{
		Target = NewTarget;
		OnLockOnChanged.Broadcast();
	}
}

bool UAgLockOnComponent::IsTargetLost() const
{
	const AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(GetOwner());
	const UAgPlayerData* Data = Player ? Cast<UAgPlayerData>(Player->GetCharacterData()) : nullptr;
	const AAgBossCharacter* Boss = Target.Get();
	return !Boss || !Data || Boss->IsDead() || UAgCombatLibrary::GetHorizontalDistance(Player, Boss) > Data->LockOnReleaseRange;
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
