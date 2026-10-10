// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_HitReaction.h"

#include "Abilities/Tasks/AbilityTask_ApplyRootMotionJumpForce.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitMovementModeChange.h"
#include "Abilities/Tasks/AbilityTask_WaitVelocityChange.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/AgCharacterBase.h"
#include "Combat/AgCombatLibrary.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "Data/AgCombatRules.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	const FName KnockUpLandSection(TEXT("Land"));
}

UAgAbility_HitReaction::UAgAbility_HitReaction()
{
	bRetriggerInstancedAbility = true;

	ActivationOwnedTags.AddTag(AgGameplayTags::State_HitReaction);
	// Same priority (상태 우선순위 4): the newer reaction replaces the older one. Actions stop.
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Action);
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Reaction_KnockBack);
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Reaction_Down);
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Reaction_KnockUp);
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Reaction_GuardPushback);
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Reaction_GuardBreak);

	ActivationBlockedTags.AddTag(AgGameplayTags::State_Groggy);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Execution_Executing);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Execution_Executed);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Dead);
}

void UAgAbility_HitReaction::SetupReaction(const FGameplayTag& AbilityTag, const FGameplayTag& TriggerEvent)
{
	SetAssetTags(FGameplayTagContainer(AbilityTag));

	FAbilityTriggerData& Trigger = AbilityTriggers.AddDefaulted_GetRef();
	Trigger.TriggerTag = TriggerEvent;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
}

void UAgAbility_HitReaction::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UAnimMontage* Montage = GetReactionMontage();
	AAgCharacterBase* Character = GetAgCharacter();
	if (!Montage || !Character || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 다운, 넉업: invincible from the moment the body is on the ground until getting up ends.
	UAbilityTask_WaitGameplayEvent* GroundedTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Event_DownGrounded, nullptr, /*OnlyTriggerOnce*/ true);
	GroundedTask->EventReceived.AddDynamic(this, &ThisClass::HandleGrounded);
	GroundedTask->ReadyForActivation();

	// Turn toward the attack so the motion pushes away from it (전투 시스템 '피격 반응').
	if (const AActor* Attacker = TriggerEventData ? TriggerEventData->Instigator.Get() : nullptr)
	{
		const FVector ToAttacker = -UAgCombatLibrary::GetAttackDirection(Attacker, Character);
		if (!ToAttacker.IsNearlyZero())
		{
			Character->SetActorRotation(FRotator(0.f, ToAttacker.Rotation().Yaw, 0.f));
		}
	}

	bPlayingFollowUp = false;
	PlayMontage(Montage);
}

void UAgAbility_HitReaction::PlayMontage(UAnimMontage* Montage, FName StartSection)
{
	// A replaced task must not end the ability when its montage stops.
	if (MontageTask)
	{
		MontageTask->EndTask();
	}
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage, 1.f, StartSection);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::HandleMontageBlendingOut);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
}

bool UAgAbility_HitReaction::PlayFollowUp()
{
	UAnimMontage* FollowUp = bPlayingFollowUp ? nullptr : GetFollowUpMontage();
	if (!FollowUp)
	{
		return false;
	}
	bPlayingFollowUp = true;
	PlayMontage(FollowUp);
	return true;
}

void UAgAbility_HitReaction::HandleMontageBlendingOut()
{
	// The follow-up (getting up) cross-fades in as the motion blends out. The reaction itself lasts until the last motion
	// ends (전투 시스템 '피격 모션': no action and, for 다운 and 넉업, invincible until getting up ends).
	PlayFollowUp();
}

void UAgAbility_HitReaction::HandleMontageCompleted()
{
	if (PlayFollowUp())
	{
		return;
	}
	MontageTask = nullptr;
	EndSelf(false);
}

void UAgAbility_HitReaction::HandleMontageCancelled()
{
	MontageTask = nullptr;
	EndSelf(true);
}

void UAgAbility_HitReaction::HandleGrounded(FGameplayEventData Payload)
{
	if (!bInvincible)
	{
		bInvincible = true;
		GetAbilitySystemComponentFromActorInfo()->AddLooseGameplayTag(AgGameplayTags::State_Invincible);
	}
}

void UAgAbility_HitReaction::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bInvincible)
	{
		bInvincible = false;
		GetAbilitySystemComponentFromActorInfo()->RemoveLooseGameplayTag(AgGameplayTags::State_Invincible);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

UAgAbility_KnockBack::UAgAbility_KnockBack()
{
	SetupReaction(AgGameplayTags::Ability_Reaction_KnockBack, AgGameplayTags::Event_Hit_KnockBack);
}

UAnimMontage* UAgAbility_KnockBack::GetReactionMontage() const
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	return Data ? Data->KnockBackMontage.Get() : nullptr;
}

UAgAbility_Down::UAgAbility_Down()
{
	SetupReaction(AgGameplayTags::Ability_Reaction_Down, AgGameplayTags::Event_Hit_Down);
}

UAnimMontage* UAgAbility_Down::GetReactionMontage() const
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	return Data ? Data->DownMontage.Get() : nullptr;
}

UAnimMontage* UAgAbility_Down::GetFollowUpMontage() const
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	return Data ? Data->GetUpMontage.Get() : nullptr;
}

UAgAbility_KnockUp::UAgAbility_KnockUp()
{
	SetupReaction(AgGameplayTags::Ability_Reaction_KnockUp, AgGameplayTags::Event_Hit_KnockUp);
}

UAnimMontage* UAgAbility_KnockUp::GetReactionMontage() const
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	return Data ? Data->KnockUpMontage.Get() : nullptr;
}

UAnimMontage* UAgAbility_KnockUp::GetFollowUpMontage() const
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	return Data ? Data->GetUpMontage.Get() : nullptr;
}

UAnimMontage* UAgAbility_KnockUp::GetFallMontage() const
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	return Data ? Data->KnockUpFallMontage.Get() : nullptr;
}

void UAgAbility_KnockUp::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	bFalling = false;
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (!IsActive())
	{
		return;
	}

	const UAgCombatRules* Rules = UAgCombatLibrary::GetCombatRules();
	const UAnimMontage* FallMontage = GetFallMontage();
	UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance();
	if (!Rules || !FallMontage || !AnimInstance || Rules->KnockUpAirTime <= 0.f)
	{
		EndSelf(true);
		return;
	}

	// The character movement drops other root motion while a montage gives root motion, so the motions' own root motion
	// is set aside until the reaction ends: the jump moves the character, and the landing plays where it fell.
	bRootMotionIgnored = true;
	SavedRootMotionMode = AnimInstance->RootMotionMode;
	AnimInstance->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);

	// 전투 시스템 '넉업': straight up from where the hit landed and back down along a parabola of the height and air time,
	// whatever the gravity. A landing ends the jump; a hit in the air keeps falling at the path's last speed until then.
	const float AirTime = Rules->KnockUpAirTime;
	UAbilityTask_ApplyRootMotionJumpForce* JumpTask = UAbilityTask_ApplyRootMotionJumpForce::ApplyRootMotionJumpForce(this, NAME_None,
		GetAgCharacter()->GetActorRotation(), /*Distance*/ 0.f, Rules->KnockUpHeight, AirTime, /*MinimumLandedTriggerTime*/ 0.5f,
		/*bFinishOnLanded*/ true, ERootMotionFinishVelocityMode::SetVelocity, FVector::ZeroVector, 0.f, nullptr, nullptr);
	JumpTask->ReadyForActivation();

	// The jump steers the velocity from the next movement update. Start the flight now at the path's first speed so the
	// fall check below never sees the speed from before the hit.
	UCharacterMovementComponent* Movement = GetAgCharacter()->GetCharacterMovement();
	Movement->SetMovementMode(MOVE_Falling);
	Movement->Velocity = FVector(0.f, 0.f, 4.f * Rules->KnockUpHeight / AirTime);

	// The falling pose's blend is centered on the top of the flight: it starts that half a blend before the top. The
	// path's rise slows like gravity of 8 × height ÷ air time².
	const float PathGravity = 8.f * Rules->KnockUpHeight / FMath::Square(AirTime);
	const float FallBlendSpeed = PathGravity * FallMontage->BlendIn.GetBlendTime() * 0.5f;
	UAbilityTask_WaitVelocityChange* FallTask = UAbilityTask_WaitVelocityChange::CreateWaitVelocityChange(this, FVector::DownVector, -FallBlendSpeed);
	FallTask->OnVelocityChage.AddDynamic(this, &ThisClass::HandleFalling);
	FallTask->ReadyForActivation();

	UAbilityTask_WaitMovementModeChange* LandTask = UAbilityTask_WaitMovementModeChange::CreateWaitMovementModeChange(this, MOVE_Walking);
	LandTask->OnChange.AddDynamic(this, &ThisClass::HandleLanded);
	LandTask->ReadyForActivation();
}

void UAgAbility_KnockUp::HandleFalling()
{
	if (!bFalling)
	{
		bFalling = true;
		PlayMontage(GetFallMontage());
	}
}

void UAgAbility_KnockUp::HandleLanded(EMovementMode NewMovementMode)
{
	// The falling pose and the landing motion's first pose are the same, so the jump to Land doesn't show.
	// A landing before the falling pose started plays the montage from Land.
	UAnimMontage* FallMontage = GetFallMontage();
	UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance();
	if (bFalling && AnimInstance && AnimInstance->Montage_IsPlaying(FallMontage))
	{
		AnimInstance->Montage_JumpToSection(KnockUpLandSection, FallMontage);
		return;
	}
	bFalling = true;
	PlayMontage(FallMontage, KnockUpLandSection);
}

void UAgAbility_KnockUp::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bRootMotionIgnored)
	{
		bRootMotionIgnored = false;
		if (UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance())
		{
			AnimInstance->SetRootMotionMode(SavedRootMotionMode);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

UAgAbility_GuardPushback::UAgAbility_GuardPushback()
{
	SetupReaction(AgGameplayTags::Ability_Reaction_GuardPushback, AgGameplayTags::Event_Guard_Pushback);
	ActivationOwnedTags.AddTag(AgGameplayTags::State_Guarding);
}

UAnimMontage* UAgAbility_GuardPushback::GetReactionMontage() const
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	return Data ? Data->GuardPushbackMontage.Get() : nullptr;
}

UAgAbility_GuardBreak::UAgAbility_GuardBreak()
{
	SetupReaction(AgGameplayTags::Ability_Reaction_GuardBreak, AgGameplayTags::Event_Guard_Break);
}

UAnimMontage* UAgAbility_GuardBreak::GetReactionMontage() const
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	return Data ? Data->GuardBreakMontage.Get() : nullptr;
}
