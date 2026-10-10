// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgBossPattern_Leap.h"

#include "Abilities/Tasks/AbilityTask_WaitMovementModeChange.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Character/AgBossCharacter.h"
#include "Combat/AgCombatLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgAttackData.h"
#include "Data/AgCharacterData.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const FName LeapEndSection(TEXT("End"));
}

void UAgBossPattern_Leap::OnPatternStarted()
{
	AAgBossCharacter* Boss = GetBoss();
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	const AActor* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Boss || !Data || !Player)
	{
		EndSelf(true);
		return;
	}
	bLanded = false;

	// 착지 목표: short of the player's position at the leap, toward the boss, within the maximum leap distance.
	const FVector Start = Boss->GetActorLocation();
	const FVector ToPlayer = FVector(Player->GetActorLocation() - Start) * FVector(1.f, 1.f, 0.f);
	const FVector Direction = ToPlayer.GetSafeNormal();
	const float Distance = FMath::Clamp(ToPlayer.Size() - Data->LeapTargetOffset, 0.f, Data->LeapMaxDistance);

	// 궤적: the engine's gravity over the air time (shorter in phase 2) decides the launch speed and the peak height.
	const float AirTime = Data->LeapAirTime / Boss->GetPatternSpeed();
	const float Gravity = -Boss->GetCharacterMovement()->GetGravityZ();
	const FVector Launch = Direction * (Distance / AirTime) + FVector(0.f, 0.f, Gravity * AirTime * 0.5f);
	if (!Direction.IsNearlyZero())
	{
		Boss->SetActorRotation(FRotator(0.f, Direction.Rotation().Yaw, 0.f));
	}
	SetRotationLocked(true);
	Boss->LaunchCharacter(Launch, /*bXYOverride*/ true, /*bZOverride*/ true);

	UAbilityTask_WaitMovementModeChange* LandTask = UAbilityTask_WaitMovementModeChange::CreateWaitMovementModeChange(this, MOVE_Walking);
	LandTask->OnChange.AddDynamic(this, &ThisClass::HandleLanded);
	LandTask->ReadyForActivation();
}

void UAgBossPattern_Leap::HandleLanded(EMovementMode NewMovementMode)
{
	AAgBossCharacter* Boss = GetBoss();
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	const FAgBossPattern* Pattern = GetPattern();
	if (bLanded || !Boss || !Data || !Pattern || Pattern->Hits.IsEmpty())
	{
		return;
	}
	bLanded = true;
	SetRotationLocked(false);

	if (UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance())
	{
		AnimInstance->Montage_JumpToSection(LeapEndSection, GetMontage());
	}

	// The landing is the attack window: one check of the landing radius.
	AAgCharacterBase* Player = Cast<AAgCharacterBase>(UGameplayStatics::GetPlayerPawn(this, 0));
	const float HeightGap = Player ? FMath::Abs(Player->GetActorLocation().Z - Boss->GetActorLocation().Z) : 0.f;
	if (Player && UAgCombatLibrary::GetHorizontalDistance(Boss, Player) <= Data->LeapHitRadius && HeightGap <= Boss->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2.f)
	{
		UAgCombatLibrary::ProcessHit(Boss, Player, Pattern->Hits[0], Boss->GetActorLocation());
	}
}

void UAgBossPattern_Leap::SetRotationLocked(bool bLocked)
{
	if (bRotationLocked == bLocked)
	{
		return;
	}
	bRotationLocked = bLocked;
	if (bLocked)
	{
		GetAbilitySystemComponentFromActorInfo()->AddLooseGameplayTag(AgGameplayTags::State_Boss_RotationLocked);
	}
	else
	{
		GetAbilitySystemComponentFromActorInfo()->RemoveLooseGameplayTag(AgGameplayTags::State_Boss_RotationLocked);
	}
}

void UAgBossPattern_Leap::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	SetRotationLocked(false);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
