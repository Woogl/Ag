// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgBossPattern_Leap.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitMovementModeChange.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Character/AgBossCharacter.h"
#include "Combat/AgCombatLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgAttackData.h"
#include "Data/AgCameraData.h"
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
	Boss->LaunchCharacter(Launch, /*bXYOverride*/ true, /*bZOverride*/ true);

	// 회전: the boss keeps turning toward the player in the air and stops 회전 정지 before the landing (its attack window).
	// Phase 2 shortens both by the same ratio.
	const float TurnTime = (Data->LeapAirTime - Data->RotationStopLead) / Boss->GetPatternSpeed();
	if (TurnTime > 0.f)
	{
		UAbilityTask_WaitDelay* TurnTask = UAbilityTask_WaitDelay::WaitDelay(this, TurnTime);
		TurnTask->OnFinish.AddDynamic(this, &ThisClass::HandleRotationStop);
		TurnTask->ReadyForActivation();
	}
	else
	{
		SetRotationLocked(true);
	}

	// 예고 섬광: the lead before the landing, until it lands (the same phase 2 rule).
	const FAgBossPattern* Pattern = GetPattern();
	if (Pattern && !Pattern->Hits.IsEmpty() && Pattern->Hits[0].bParryable)
	{
		const float FlashTime = (Data->LeapAirTime - Data->ParryFlashLead) / Boss->GetPatternSpeed();
		if (FlashTime > 0.f)
		{
			UAbilityTask_WaitDelay* FlashTask = UAbilityTask_WaitDelay::WaitDelay(this, FlashTime);
			FlashTask->OnFinish.AddDynamic(this, &ThisClass::HandleParryFlashTime);
			FlashTask->ReadyForActivation();
		}
		else
		{
			SetParryFlash(true);
		}
	}

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
	SetParryFlash(false);

	if (UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance())
	{
		AnimInstance->Montage_JumpToSection(LeapEndSection, GetMontage());
	}

	// The landing shakes the camera when the player is near enough (카메라 '상황별 단계').
	AAgCharacterBase* Player = Cast<AAgCharacterBase>(UGameplayStatics::GetPlayerPawn(this, 0));
	const UAgCameraData* Camera = UAgCombatLibrary::GetCameraData();
	if (Player && Camera && UAgCombatLibrary::GetHorizontalDistance(Boss, Player) <= Camera->LeapLandingShakeRange)
	{
		UAgCombatLibrary::PlayCameraShake(Boss, Camera->LeapLandingShake);
	}

	// The landing is the attack window: one check of the landing radius.
	const float HeightGap = Player ? FMath::Abs(Player->GetActorLocation().Z - Boss->GetActorLocation().Z) : 0.f;
	if (Player && UAgCombatLibrary::GetHorizontalDistance(Boss, Player) <= Data->LeapHitRadius && HeightGap <= Boss->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2.f)
	{
		UAgCombatLibrary::ProcessHit(Boss, Player, Pattern->Hits[0], Boss->GetActorLocation());
	}
}

void UAgBossPattern_Leap::HandleParryFlashTime()
{
	if (!bLanded)
	{
		SetParryFlash(true);
	}
}

void UAgBossPattern_Leap::HandleRotationStop()
{
	if (!bLanded)
	{
		SetRotationLocked(true);
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
