// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_Dodge.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/AgEffect_Cost.h"
#include "AbilitySystem/AgEffect_Resource.h"
#include "AbilitySystem/AgEffect_TimedTag.h"
#include "AbilitySystem/Tasks/AgAbilityTask_HoldTag.h"
#include "AbilitySystem/Tasks/AgAbilityTask_WaitMontagePosition.h"
#include "AbilitySystemComponent.h"
#include "Ag.h"
#include "Character/AgPlayerCharacter.h"
#include "Combat/AgTimeSubsystem.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"

UAgAbility_Dodge::UAgAbility_Dodge()
{
	SetupPlayerAction(AgGameplayTags::Ability_Action_Dodge);
	SetCooldownTag(AgGameplayTags::Cooldown_Dodge);
	CostGameplayEffectClass = UAgEffect_Cost::StaticClass();

	// 회피 후딜 allows a new dodge: pressing it again restarts this ability.
	bRetriggerInstancedAbility = true;
}

void UAgAbility_Dodge::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(GetAgCharacter());
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	if (!Player || !Data)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const float SPBefore = Player->GetAttributeSet()->GetSP();
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	SPSpent = SPBefore - Player->GetAttributeSet()->GetSP();
	bPerfectDodged = false;

	// 방향: toward the movement input whether locked on or not; without input, step back from where the character faces.
	const FVector InputDirection = Player->GetMoveInputDirection();
	const bool bRoll = !InputDirection.IsNearlyZero();
	if (bRoll)
	{
		Player->SetActorRotation(FRotator(0.f, InputDirection.Rotation().Yaw, 0.f));
	}
	UAnimMontage* Montage = bRoll ? Data->RollMontage.Get() : Data->BackstepMontage.Get();
	if (!Montage)
	{
		EndSelf(true);
		return;
	}

	BeginBusy();
	InvincibleTag = UAgAbilityTask_HoldTag::HoldTag(this, AgGameplayTags::State_Invincible);
	InvincibleTag->ReadyForActivation();
	PerfectDodgeWindowTag = UAgAbilityTask_HoldTag::HoldTag(this, AgGameplayTags::State_PerfectDodgeWindow);
	PerfectDodgeWindowTag->ReadyForActivation();

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->ReadyForActivation();

	UAgAbilityTask_WaitMontagePosition* PerfectDodgeTask = UAgAbilityTask_WaitMontagePosition::WaitMontagePosition(this, Montage, Data->PerfectDodgeTime);
	PerfectDodgeTask->OnReached.AddDynamic(this, &ThisClass::HandlePerfectDodgeWindowEnded);
	PerfectDodgeTask->ReadyForActivation();

	UAgAbilityTask_WaitMontagePosition* InvincibleTask = UAgAbilityTask_WaitMontagePosition::WaitMontagePosition(this, Montage, Data->DodgeInvincibleTime);
	InvincibleTask->OnReached.AddDynamic(this, &ThisClass::HandleInvincibilityEnded);
	InvincibleTask->ReadyForActivation();

	UAgAbilityTask_WaitMontagePosition* RecoveryTask = UAgAbilityTask_WaitMontagePosition::WaitMontagePosition(this, Montage, bRoll ? Data->RollRecoveryStart : Data->BackstepRecoveryStart);
	RecoveryTask->OnReached.AddDynamic(this, &ThisClass::HandleRecoveryStarted);
	RecoveryTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* PerfectDodgeEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Event_PerfectDodge);
	PerfectDodgeEvent->EventReceived.AddDynamic(this, &ThisClass::HandlePerfectDodge);
	PerfectDodgeEvent->ReadyForActivation();
}

void UAgAbility_Dodge::HandlePerfectDodgeWindowEnded()
{
	PerfectDodgeWindowTag->EndTask();
}

void UAgAbility_Dodge::HandleInvincibilityEnded()
{
	InvincibleTag->EndTask();
}

void UAgAbility_Dodge::HandleRecoveryStarted()
{
	EndBusy();
}

void UAgAbility_Dodge::HandlePerfectDodge(FGameplayEventData Payload)
{
	// 회피 한 번에 한 번만 받습니다. 그 뒤의 공격도 피하지만 아무것도 주지 않습니다.
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	AAgCharacterBase* Character = GetAgCharacter();
	if (bPerfectDodged || !Data || !Character)
	{
		return;
	}
	bPerfectDodged = true;
	UE_LOG(LogAg, Verbose, TEXT("Perfect dodge: SP +%.0f, MP +%.0f, UP +%.0f"), SPSpent, Data->PerfectDodgeMPCharge, Data->PerfectDodgeUPCharge);
	const FGameplayEffectSpecHandle Reward = MakeOutgoingGameplayEffectSpec(UAgEffect_Resource::StaticClass());
	UAgEffect_Resource::SetAmounts(*Reward.Data, SPSpent, Data->PerfectDodgeMPCharge, Data->PerfectDodgeUPCharge);
	ApplyGameplayEffectSpecToOwner(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), Reward);

	// 회피 반격 기회는 이때부터 시작해 회피가 끝나도 이어집니다.
	UAgEffect_TimedTag::Apply(GetAbilitySystemComponentFromActorInfo(), AgGameplayTags::State_DodgeCounterChance, Data->DodgeCounterChanceTime);

	if (UAgTimeSubsystem* TimeSubsystem = UWorld::GetSubsystem<UAgTimeSubsystem>(Character->GetWorld()))
	{
		TimeSubsystem->StartSlowMotion(Data->SlowMotionSpeed, Data->SlowMotionDuration);
	}
}

void UAgAbility_Dodge::HandleMontageFinished()
{
	// 회피는 모션이 끝날 때 끝납니다. 달리기 전환은 후딜에서 플레이어 캐릭터가 맡습니다.
	EndSelf(false);
}

void UAgAbility_Dodge::HandleMontageCancelled()
{
	EndSelf(true);
}

float UAgAbility_Dodge::GetCooldownDuration() const
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	return Data ? Data->DodgeCooldown : 0.f;
}

bool UAgAbility_Dodge::CanPayCost(const UAgAttributeSet& Stats) const
{
	// Usable while SP is above 0, even below the cost.
	return Stats.GetSP() > 0.f;
}

FAgAbilityCost UAgAbility_Dodge::GetCost() const
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	return { Data ? Data->DodgeSPCost : 0.f, 0.f, 0.f };
}
