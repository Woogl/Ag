// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_Dodge.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/Tasks/AgAbilityTask_WaitMontagePosition.h"
#include "AbilitySystemComponent.h"
#include "Ag.h"
#include "Character/AgPlayerCharacter.h"
#include "Combat/AgCombatLibrary.h"
#include "Combat/AgTimeSubsystem.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"

UAgAbility_Dodge::UAgAbility_Dodge()
{
	SetupPlayerAction(AgGameplayTags::Ability_Action_Dodge);
	SetCooldownTag(AgGameplayTags::Cooldown_Dodge);

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
	SetStateTag(AgGameplayTags::State_Invincible, bInvincible, true);
	SetStateTag(AgGameplayTags::State_PerfectDodgeWindow, bInPerfectDodgeWindow, true);

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

void UAgAbility_Dodge::SetStateTag(const FGameplayTag& Tag, bool& bHasTag, bool bWanted)
{
	if (bHasTag == bWanted)
	{
		return;
	}
	bHasTag = bWanted;
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (bWanted)
	{
		ASC->AddLooseGameplayTag(Tag);
	}
	else
	{
		ASC->RemoveLooseGameplayTag(Tag);
	}
}

void UAgAbility_Dodge::HandlePerfectDodgeWindowEnded()
{
	SetStateTag(AgGameplayTags::State_PerfectDodgeWindow, bInPerfectDodgeWindow, false);
}

void UAgAbility_Dodge::HandleInvincibilityEnded()
{
	SetStateTag(AgGameplayTags::State_Invincible, bInvincible, false);
}

void UAgAbility_Dodge::HandleRecoveryStarted()
{
	EndBusy();
}

void UAgAbility_Dodge::HandlePerfectDodge(FGameplayEventData Payload)
{
	// Once per dodge; later attacks are still dodged but give nothing.
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	AAgCharacterBase* Character = GetAgCharacter();
	if (bPerfectDodged || !Data || !Character)
	{
		return;
	}
	bPerfectDodged = true;
	UE_LOG(LogAg, Verbose, TEXT("Perfect dodge: SP +%.0f, MP +%.0f, UP +%.0f"), SPSpent, Data->PerfectDodgeMPCharge, Data->PerfectDodgeUPCharge);
	UAgCombatLibrary::ApplyResourceChange(Character, { SPSpent, Data->PerfectDodgeMPCharge, Data->PerfectDodgeUPCharge });

	// 회피 반격 기회 from this moment; it outlives the dodge.
	UAgCombatLibrary::ApplyTimedTag(GetAbilitySystemComponentFromActorInfo(), AgGameplayTags::State_DodgeCounterChance, Data->DodgeCounterChanceTime);

	if (UAgTimeSubsystem* TimeSubsystem = UWorld::GetSubsystem<UAgTimeSubsystem>(Character->GetWorld()))
	{
		TimeSubsystem->StartSlowMotion(Data->SlowMotionSpeed, Data->SlowMotionDuration);
	}
}

void UAgAbility_Dodge::HandleMontageFinished()
{
	// The dodge lasts to the end of its motion; with Shift and movement still held it turns into a sprint.
	AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(GetAgCharacter());
	EndSelf(false);
	if (Player)
	{
		Player->TryStartSprint();
	}
}

void UAgAbility_Dodge::HandleMontageCancelled()
{
	EndSelf(true);
}

void UAgAbility_Dodge::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	SetStateTag(AgGameplayTags::State_PerfectDodgeWindow, bInPerfectDodgeWindow, false);
	SetStateTag(AgGameplayTags::State_Invincible, bInvincible, false);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
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

FAgResourceAmounts UAgAbility_Dodge::GetCost() const
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	return { Data ? Data->DodgeSPCost : 0.f, 0.f, 0.f };
}
