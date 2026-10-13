// Copyright Woogle. All Rights Reserved.

#include "Character/AgLockOnComponent.h"

#include "Character/AgPlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/AgCharacterData.h"
#include "GameFramework/PlayerController.h"

AAgCharacterBase* UAgLockOnComponent::FindTarget() const
{
	const AAgPlayerCharacter* Player = CastChecked<AAgPlayerCharacter>(GetOwner());
	const UAgPlayerData* Data = CastChecked<UAgPlayerData>(Player->GetCharacterData());
	const APlayerController* PlayerController = Cast<APlayerController>(Player->GetController());
	if (!PlayerController)
	{
		return nullptr;
	}

	// 화면 안에 있는 적만 락온할 수 있습니다.
	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;
	PlayerController->GetViewportSize(ViewportWidth, ViewportHeight);
	return Player->FindEnemy(Data->LockOnRange, [PlayerController, ViewportWidth, ViewportHeight](const AAgCharacterBase& Enemy)
	{
		FVector2D ScreenPosition;
		return PlayerController->ProjectWorldLocationToScreen(Enemy.GetActorLocation(), ScreenPosition)
			&& ScreenPosition.X >= 0.f && ScreenPosition.Y >= 0.f && ScreenPosition.X <= ViewportWidth && ScreenPosition.Y <= ViewportHeight;
	});
}

void UAgLockOnComponent::SetTarget(AAgCharacterBase* NewTarget)
{
	// 해제해도 카메라는 그대로 둡니다. 자유 시점이 지금 보는 방향에서 이어집니다.
	if (Target.Get() != NewTarget)
	{
		Target = NewTarget;
		OnLockOnChanged.Broadcast();
	}
}

bool UAgLockOnComponent::IsTargetLost() const
{
	const AAgPlayerCharacter* Player = CastChecked<AAgPlayerCharacter>(GetOwner());
	const UAgPlayerData* Data = CastChecked<UAgPlayerData>(Player->GetCharacterData());
	const AAgCharacterBase* TargetCharacter = Target.Get();
	return !TargetCharacter || TargetCharacter->IsDead() || Player->GetHorizontalDistanceTo(TargetCharacter) > Data->LockOnReleaseRange;
}

FVector UAgLockOnComponent::GetLockOnPoint() const
{
	// 락온 지점 본은 보스 데이터에 있습니다. 본이 없는 대상은 캡슐 중심을 봅니다.
	const AAgCharacterBase* TargetCharacter = Target.Get();
	const UAgBossData* Data = TargetCharacter ? Cast<UAgBossData>(TargetCharacter->GetCharacterData()) : nullptr;
	if (Data && !Data->LockOnBone.IsNone() && TargetCharacter->GetMesh()->GetBoneIndex(Data->LockOnBone) != INDEX_NONE)
	{
		return TargetCharacter->GetMesh()->GetBoneLocation(Data->LockOnBone);
	}
	return TargetCharacter ? TargetCharacter->GetActorLocation() : FVector::ZeroVector;
}
