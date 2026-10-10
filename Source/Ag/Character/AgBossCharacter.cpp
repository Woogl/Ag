// Copyright Woogle. All Rights Reserved.

#include "Character/AgBossCharacter.h"

#include "AbilitySystem/Abilities/AgBossPatternAbility.h"
#include "AbilitySystemComponent.h"
#include "AI/AgBossAIController.h"
#include "Animation/AgAnimNotifyState_AttackWindow.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AAgBossCharacter::AAgBossCharacter()
{
	AIControllerClass = AAgBossAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// The boss turns toward the player by itself, not toward where it walks.
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
}

void AAgBossCharacter::GrantAbilities()
{
	Super::GrantAbilities();

	const UAgBossData* Data = Cast<UAgBossData>(GetCharacterData());
	if (!Data)
	{
		return;
	}
	for (const FAgBossPattern& Pattern : Data->Patterns)
	{
		if (Pattern.AbilityClass && Pattern.Pattern.IsValid())
		{
			FGameplayAbilitySpec Spec(Pattern.AbilityClass);
			Spec.GetDynamicSpecSourceTags().AddTag(Pattern.Pattern);
			GetAbilitySystemComponent()->GiveAbility(Spec);
		}
	}
}

FGameplayAbilitySpec* AAgBossCharacter::FindPatternSpec(const FGameplayTag& Pattern) const
{
	for (FGameplayAbilitySpec& Spec : GetAbilitySystemComponent()->GetActivatableAbilities())
	{
		if (UAgBossPatternAbility::GetPatternTag(Spec) == Pattern)
		{
			return &Spec;
		}
	}
	return nullptr;
}

void AAgBossCharacter::EnterPhase2()
{
	Phase = 2;
	UpdateMoveSpeed();
}

float AAgBossCharacter::GetPatternSpeed() const
{
	const UAgBossData* Data = Cast<UAgBossData>(GetCharacterData());
	return (Data && Phase >= 2) ? Data->Phase2PatternSpeed : 1.f;
}

float AAgBossCharacter::GetMoveSpeedMultiplier() const
{
	const UAgBossData* Data = Cast<UAgBossData>(GetCharacterData());
	return (Data && Phase >= 2) ? Data->Phase2MOVMultiplier : 1.f;
}

void AAgBossCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateRotation(DeltaSeconds);
}

void AAgBossCharacter::UpdateRotation(float DeltaSeconds)
{
	const UAgBossData* Data = Cast<UAgBossData>(GetCharacterData());
	const AActor* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (IsDead() || !Data || !Player)
	{
		return;
	}

	FGameplayTagContainer NoTurning;
	NoTurning.AddTag(AgGameplayTags::State_Groggy);
	NoTurning.AddTag(AgGameplayTags::State_Execution_Executed);
	NoTurning.AddTag(AgGameplayTags::State_Boss_RotationLocked);
	if (GetAbilitySystemComponent()->HasAnyMatchingGameplayTags(NoTurning))
	{
		return;
	}

	if (IsInRotationStopWindow())
	{
		// The first-hit approach target is fixed where the player is when the boss stops turning.
		StopApproach();
		return;
	}

	const FVector ToPlayer = (Player->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (!ToPlayer.IsNearlyZero())
	{
		const FRotator Goal(0.f, ToPlayer.Rotation().Yaw, 0.f);
		SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(), Goal, DeltaSeconds, Data->RotationSpeed));
	}
}

bool AAgBossCharacter::IsInRotationStopWindow() const
{
	const UAgBossData* Data = Cast<UAgBossData>(GetCharacterData());
	const UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!Data || !AnimInstance)
	{
		return false;
	}

	// The lead is in seconds at the pattern play rate; in montage time it scales with that rate.
	// Phase 2 speeds up both the montage and the lead by the same factor, so this montage-time lead doesn't change.
	const float LeadInMontageTime = Data->RotationStopLead * Data->PatternPlayRate;
	for (const FAnimMontageInstance* Instance : AnimInstance->MontageInstances)
	{
		if (!Instance || !Instance->Montage || !Instance->IsActive())
		{
			continue;
		}
		const float Position = Instance->GetPosition();
		for (const FAnimNotifyEvent& Notify : Instance->Montage->Notifies)
		{
			if (Cast<UAgAnimNotifyState_AttackWindow>(Notify.NotifyStateClass)
				&& Position >= Notify.GetTriggerTime() - LeadInMontageTime && Position <= Notify.GetEndTriggerTime())
			{
				return true;
			}
		}
	}
	return false;
}
