// Copyright Woogle. All Rights Reserved.

#include "Character/AgEnemySightPreviewComponent.h"

#if WITH_EDITOR
#include "Character/AgEnemyCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Data/AgCharacterData.h"
#include "Game/AgEnemySpawner.h"
#endif

UAgEnemySightPreviewComponent::UAgEnemySightPreviewComponent()
{
	bIsEditorOnly = true;
}

#if WITH_EDITOR
bool UAgEnemySightPreviewComponent::GetSightPreview(FVector& OutOrigin, FQuat& OutRotation, float& OutDistance, float& OutAngle) const
{
	const AAgEnemySpawner* Spawner = Cast<AAgEnemySpawner>(GetOwner());
	const AAgEnemyCharacter* Enemy = Spawner ? Spawner->GetEnemyDefaults() : nullptr;
	const UAgCharacterData* Data = Enemy ? UAgCharacterData::Find(Enemy->GetCharacterDataId()) : nullptr;
	if (!Data)
	{
		return false;
	}
	const FAgEnemyAIData& AI = Data->AI;
	if (!FMath::IsFinite(AI.SightDistance) || !FMath::IsFinite(AI.SightAngle) || AI.SightDistance <= 0.f || AI.SightAngle <= 0.f || AI.SightAngle > 360.f)
	{
		return false;
	}
	// 적은 캡슐 중심에서 몸의 정면으로 봅니다(AAgEnemyCharacter::GetActorEyesViewPoint). 스포너의 위치는 적의 발밑입니다.
	OutOrigin = Spawner->GetActorLocation() + Spawner->GetActorUpVector() * Enemy->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	OutRotation = Spawner->GetActorQuat();
	OutDistance = AI.SightDistance;
	OutAngle = AI.SightAngle;
	return true;
}
#endif
