// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgBossPattern_Dash.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/Tasks/AgAbilityTask_Dash.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Character/AgBossCharacter.h"
#include "Combat/AgCombatLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "Kismet/GameplayStatics.h"

void UAgBossPattern_Dash::OnPatternStarted()
{
	bDashing = bHolding = bRootMotionIgnored = false;

	UAbilityTask_WaitGameplayEvent* StartTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Event_Boss_DashStart, nullptr, /*OnlyTriggerOnce*/ true);
	StartTask->EventReceived.AddDynamic(this, &ThisClass::HandleDashStart);
	StartTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* HoldTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Event_Boss_DashHold, nullptr, /*OnlyTriggerOnce*/ true);
	HoldTask->EventReceived.AddDynamic(this, &ThisClass::HandleDashHold);
	HoldTask->ReadyForActivation();
}

void UAgBossPattern_Dash::GetUnguardableHitStarts(TArray<float>& OutTimes) const
{
	// The attack window opens when the dash starts.
	const FAgBossPattern* Pattern = GetPattern();
	const float DashStart = FindEventNotifyTime(AgGameplayTags::Event_Boss_DashStart);
	if (Pattern && !Pattern->Hits.IsEmpty() && !Pattern->Hits[0].bGuardable && DashStart >= 0.f)
	{
		OutTimes.Add(DashStart);
	}
}

void UAgBossPattern_Dash::HandleDashStart(FGameplayEventData Payload)
{
	AAgBossCharacter* Boss = GetBoss();
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	const AActor* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Boss || !Data || !Player)
	{
		return;
	}

	// 방향: fixed toward the player at this moment. 목표 지점: past the player, up to the maximum distance.
	const FVector Direction = UAgCombatLibrary::GetAttackDirection(Boss, Player);
	if (!Direction.IsNearlyZero())
	{
		Boss->SetActorRotation(FRotator(0.f, Direction.Rotation().Yaw, 0.f));
	}
	const float Distance = FMath::Min(UAgCombatLibrary::GetHorizontalDistance(Boss, Player) + Data->DashOvershoot, Data->DashMaxDistance);

	// The character movement ignores other root motion while a montage gives root motion, so the motion's own root
	// motion is set aside from here: the dash moves the boss, and the rest of the motion only draws the spear back.
	if (UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance())
	{
		bRootMotionIgnored = true;
		SavedRootMotionMode = AnimInstance->RootMotionMode;
		AnimInstance->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
	}

	SetDashing(true);
	UAgAbilityTask_Dash* DashTask = UAgAbilityTask_Dash::Dash(this, Boss->GetActorForwardVector(), Data->DashSpeed * Boss->GetPatternSpeed(), Distance);
	DashTask->OnFinished.AddDynamic(this, &ThisClass::HandleDashFinished);
	DashTask->ReadyForActivation();
}

void UAgBossPattern_Dash::HandleDashHold(FGameplayEventData Payload)
{
	// The thrust pose holds until the dash ends.
	UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance();
	if (bDashing && AnimInstance)
	{
		bHolding = true;
		AnimInstance->Montage_Pause(GetMontage());
	}
}

void UAgBossPattern_Dash::HandleDashFinished()
{
	SetDashing(false);

	UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance();
	if (bHolding && AnimInstance)
	{
		bHolding = false;
		AnimInstance->Montage_Resume(GetMontage());
	}
}

void UAgBossPattern_Dash::SetDashing(bool bNewDashing)
{
	AAgCharacterBase* Boss = GetAgCharacter();
	if (bDashing == bNewDashing || !Boss)
	{
		return;
	}
	bDashing = bNewDashing;

	// No turning during the dash, and it passes through the player.
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (bDashing)
	{
		ASC->AddLooseGameplayTag(AgGameplayTags::State_Boss_RotationLocked);
		Boss->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		Boss->BeginAttackWindow(0);
	}
	else
	{
		ASC->RemoveLooseGameplayTag(AgGameplayTags::State_Boss_RotationLocked);
		if (!Boss->IsDead())
		{
			Boss->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		}
		Boss->EndAttackWindow(0);
	}
}

void UAgBossPattern_Dash::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	SetDashing(false);
	if (bRootMotionIgnored)
	{
		bRootMotionIgnored = false;
		if (UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance())
		{
			AnimInstance->SetRootMotionMode(SavedRootMotionMode);
		}
	}
	if (bHolding)
	{
		bHolding = false;
		if (UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance())
		{
			AnimInstance->Montage_Resume(GetMontage());
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
