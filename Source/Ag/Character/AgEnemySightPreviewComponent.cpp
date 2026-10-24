// Copyright Woogle. All Rights Reserved.

#include "Character/AgEnemySightPreviewComponent.h"

#if WITH_EDITOR
#include "Character/AgEnemyCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Core/AgAssetManager.h"
#include "Data/AgCharacterData.h"
#endif

UAgEnemySightPreviewComponent::UAgEnemySightPreviewComponent()
{
	bIsEditorOnly = true;
}

#if WITH_EDITOR
bool UAgEnemySightPreviewComponent::GetSightPreview(FVector& OutOrigin, FQuat& OutRotation, float& OutDistance, float& OutAngle) const
{
	const AAgEnemyCharacter* Enemy = Cast<AAgEnemyCharacter>(GetOwner());
	if (!Enemy)
	{
		return false;
	}
	// CharacterData는 플레이 시작 때 연결되므로 편집 중에는 ID로 정본 데이터를 조회합니다.
	const UAgEnemyData* Data = Cast<UAgEnemyData>(UAgAssetManager::Get().FindCharacterData(Enemy->GetCharacterDataId()));
	if (!Data || !FMath::IsFinite(Data->SightDistance) || !FMath::IsFinite(Data->SightAngle) || Data->SightDistance <= 0.f || Data->SightAngle <= 0.f || Data->SightAngle > 360.f)
	{
		return false;
	}
	OutOrigin = Enemy->GetCapsuleComponent()->GetComponentLocation();
	OutRotation = Enemy->GetActorQuat();
	OutDistance = Data->SightDistance;
	OutAngle = Data->SightAngle;
	return true;
}
#endif
