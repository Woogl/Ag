// Copyright Woogle. All Rights Reserved.

#include "Combat/AgCombatLibrary.h"

#include "AbilitySystemComponent.h"
#include "Character/AgCharacterBase.h"
#include "Character/AgPlayerCharacter.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCameraData.h"
#include "Kismet/GameplayStatics.h"

bool UAgCombatLibrary::AreHostile(const AAgCharacterBase* A, const AAgCharacterBase* B)
{
	return A && B && A->IsA<AAgPlayerCharacter>() != B->IsA<AAgPlayerCharacter>();
}

FVector UAgCombatLibrary::GetHorizontalDirection(const AActor* Source, const AActor* Target)
{
	if (!Source || !Target)
	{
		return FVector::ZeroVector;
	}
	return (Target->GetActorLocation() - Source->GetActorLocation()).GetSafeNormal2D();
}

float UAgCombatLibrary::GetHorizontalDistance(const AActor* A, const AActor* B)
{
	return (A && B) ? FVector::Dist2D(A->GetActorLocation(), B->GetActorLocation()) : TNumericLimits<float>::Max();
}

void UAgCombatLibrary::PlayCameraShake(const UObject* WorldContext, EAgCameraShake Shake)
{
	FGameplayTag Cue;
	switch (Shake)
	{
	case EAgCameraShake::Small:
		Cue = AgGameplayTags::GameplayCue_CameraShake_Small;
		break;
	case EAgCameraShake::Medium:
		Cue = AgGameplayTags::GameplayCue_CameraShake_Medium;
		break;
	case EAgCameraShake::Large:
		Cue = AgGameplayTags::GameplayCue_CameraShake_Large;
		break;
	default:
		return;
	}
	// 엔진 Burst 큐는 큐를 실행한 캐릭터가 로컬 플레이어일 때 그 카메라를 흔들므로, 로컬 플레이어 캐릭터에서 실행합니다.
	if (const AAgCharacterBase* Player = Cast<AAgCharacterBase>(UGameplayStatics::GetPlayerPawn(WorldContext, 0)))
	{
		Player->GetAbilitySystemComponent()->ExecuteGameplayCue(Cue, FGameplayCueParameters());
	}
}
