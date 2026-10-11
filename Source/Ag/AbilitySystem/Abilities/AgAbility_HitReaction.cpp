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
#include "Core/AgSettings.h"
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
	// 같은 우선순위(상태 우선순위 4)는 새 반응이 앞 반응을 대신합니다. 행동과 보스 패턴은 멈춥니다.
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Action);
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Boss_Pattern);
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Reaction_KnockBack);
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

	// 넉백, 넉업: 바닥에 쓰러진 순간부터 기상이 끝날 때까지 무적.
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
	// 모션이 블렌드 아웃되는 동안 이어지는 몽타주(기상)를 블렌드합니다. 반응 자체는 마지막 모션이 끝날 때까지 이어집니다
	// (전투 시스템 '피격 모션': 행동 불가, 넉백·넉업은 기상이 끝날 때까지 무적).
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

void UAgAbility_HitReaction::IgnoreMotionRootMotion()
{
	UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance();
	if (!bRootMotionIgnored && AnimInstance)
	{
		bRootMotionIgnored = true;
		SavedRootMotionMode = AnimInstance->RootMotionMode;
		AnimInstance->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
	}
}

void UAgAbility_HitReaction::RestoreMotionRootMotion()
{
	if (bRootMotionIgnored)
	{
		bRootMotionIgnored = false;
		if (UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance())
		{
			AnimInstance->SetRootMotionMode(SavedRootMotionMode);
		}
	}
}

void UAgAbility_HitReaction::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bInvincible)
	{
		bInvincible = false;
		GetAbilitySystemComponentFromActorInfo()->RemoveLooseGameplayTag(AgGameplayTags::State_Invincible);
	}
	RestoreMotionRootMotion();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

UAgAbility_KnockBack::UAgAbility_KnockBack()
{
	SetupReaction(AgGameplayTags::Ability_Reaction_KnockBack, AgGameplayTags::Event_Hit_KnockBack);
}

UAnimMontage* UAgAbility_KnockBack::GetReactionMontage() const
{
	const UAgCharacterData* Data = GetCharacterData<UAgCharacterData>();
	return Data ? Data->KnockBackMontage.Get() : nullptr;
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

	const UAgCombatRules& Rules = UAgSettings::Get()->GetCombatRules();
	const UAnimMontage* FallMontage = GetFallMontage();
	UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance();
	if (!FallMontage || !AnimInstance || Rules.KnockUpAirTime <= 0.f)
	{
		EndSelf(true);
		return;
	}

	// 반응이 끝날 때까지 모션의 루트 이동을 꺼서, 점프가 캐릭터를 옮기고 착지는 떨어진 자리에서 재생합니다.
	IgnoreMotionRootMotion();

	// 전투 시스템 '넉업': 맞은 자리에서 높이와 체공 시간의 포물선으로 곧게 오르내립니다(중력과 무관).
	// 착지하면 점프가 끝나고, 공중에서 맞았으면 착지할 때까지 경로의 마지막 속도로 떨어집니다.
	const float AirTime = Rules.KnockUpAirTime;
	UAbilityTask_ApplyRootMotionJumpForce* JumpTask = UAbilityTask_ApplyRootMotionJumpForce::ApplyRootMotionJumpForce(this, NAME_None,
		GetAgCharacter()->GetActorRotation(), /*Distance*/ 0.f, Rules.KnockUpHeight, AirTime, /*MinimumLandedTriggerTime*/ 0.5f,
		/*bFinishOnLanded*/ true, ERootMotionFinishVelocityMode::SetVelocity, FVector::ZeroVector, 0.f, nullptr, nullptr);
	JumpTask->ReadyForActivation();

	// 점프는 다음 이동 갱신부터 속도를 정하므로, 지금 경로의 첫 속도로 날기 시작해 아래 낙하 검사가 맞기 전 속도를 보지 않게 합니다.
	UCharacterMovementComponent* Movement = GetAgCharacter()->GetCharacterMovement();
	Movement->SetMovementMode(MOVE_Falling);
	Movement->Velocity = FVector(0.f, 0.f, 4.f * Rules.KnockUpHeight / AirTime);

	// 낙하 자세의 블렌드는 정점이 가운데에 오도록 정점보다 블렌드 시간의 절반 먼저 시작합니다.
	// 경로의 상승은 8 × 높이 ÷ 체공 시간² 의 중력처럼 느려집니다.
	const float PathGravity = 8.f * Rules.KnockUpHeight / FMath::Square(AirTime);
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
