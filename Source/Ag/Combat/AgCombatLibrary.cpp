// Copyright Woogle. All Rights Reserved.

#include "Combat/AgCombatLibrary.h"

#include "AbilitySystemComponent.h"
#include "Character/AgCharacterBase.h"
#include "Character/AgPlayerCharacter.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCameraData.h"

bool UAgCombatLibrary::AreHostile(const AAgCharacterBase* A, const AAgCharacterBase* B)
{
	return A && B && A->IsA<AAgPlayerCharacter>() != B->IsA<AAgPlayerCharacter>();
}

FVector UAgCombatLibrary::GetAttackDirection(const AActor* Source, const AActor* Target)
{
	if (!Source || !Target)
	{
		return FVector::ZeroVector;
	}
	return (Target->GetActorLocation() - Source->GetActorLocation()).GetSafeNormal2D();
}

void UAgCombatLibrary::TurnToward(AActor* Actor, const AActor* Target)
{
	const FVector Direction = GetAttackDirection(Actor, Target);
	if (!Direction.IsNearlyZero())
	{
		Actor->SetActorRotation(FRotator(0.f, Direction.Rotation().Yaw, 0.f));
	}
}

float UAgCombatLibrary::GetHorizontalDistance(const AActor* A, const AActor* B)
{
	return (A && B) ? FVector::Dist2D(A->GetActorLocation(), B->GetActorLocation()) : TNumericLimits<float>::Max();
}

void UAgCombatLibrary::PlayCameraShake(AAgCharacterBase* Context, EAgCameraShake Shake)
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
	if (Context)
	{
		Context->GetAbilitySystemComponent()->ExecuteGameplayCue(Cue, FGameplayCueParameters());
	}
}
