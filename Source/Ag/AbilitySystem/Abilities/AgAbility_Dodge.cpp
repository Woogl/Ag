// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_Dodge.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/AgEffect_Cost.h"
#include "AbilitySystem/AgEffect_Resource.h"
#include "AbilitySystem/AgEffect_TimedTag.h"
#include "AbilitySystem/Tasks/AgAbilityTask_HoldTag.h"
#include "AbilitySystemComponent.h"
#include "Ag.h"
#include "Character/AgPlayerCharacter.h"
#include "Combat/AgTimeSubsystem.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"

UAgAbility_Dodge::UAgAbility_Dodge()
{
	SetupPlayerAction(AgGameplayTags::Ability_Action_Dodge);
	CostGameplayEffectClass = UAgEffect_Cost::StaticClass();

	// 회피 후딜에 회피를 다시 누르면 이 어빌리티를 처음부터 다시 시작합니다.
	bRetriggerInstancedAbility = true;
}

void UAgAbility_Dodge::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AAgPlayerCharacter* Player = CastChecked<AAgPlayerCharacter>(GetAgCharacter());
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();

	const float SPBefore = Player->GetAttributeSet()->GetSP();
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	SPSpent = SPBefore - Player->GetAttributeSet()->GetSP();
	bPerfectDodged = false;

	// 방향: 락온 여부와 관계없이 이동 입력 쪽으로 구르고, 입력이 없으면 바라보는 방향에서 뒤로 물러납니다.
	const FVector InputDirection = Player->GetMoveInputDirection();
	const bool bRoll = !InputDirection.IsNearlyZero();
	if (bRoll)
	{
		Player->SetActorRotation(FRotator(0.f, InputDirection.Rotation().Yaw, 0.f));
	}
	UAnimMontage* Montage = bRoll ? Data->RollMontage.Get() : Data->BackstepMontage.Get();
	if (!Montage)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
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

	// 극한 회피 구간과 무적 구간의 끝, 후딜 시작은 회피 몽타주의 노티파이가 알립니다.
	UAbilityTask_WaitGameplayEvent* PerfectDodgeEndTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Event_PerfectDodgeWindowEnded);
	PerfectDodgeEndTask->EventReceived.AddDynamic(this, &ThisClass::HandlePerfectDodgeWindowEnded);
	PerfectDodgeEndTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* InvincibleEndTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Event_InvincibilityEnded);
	InvincibleEndTask->EventReceived.AddDynamic(this, &ThisClass::HandleInvincibilityEnded);
	InvincibleEndTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* RecoveryTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Event_RecoveryStarted);
	RecoveryTask->EventReceived.AddDynamic(this, &ThisClass::HandleRecoveryStarted);
	RecoveryTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* PerfectDodgeEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Event_PerfectDodge);
	PerfectDodgeEvent->EventReceived.AddDynamic(this, &ThisClass::HandlePerfectDodge);
	PerfectDodgeEvent->ReadyForActivation();
}

void UAgAbility_Dodge::HandlePerfectDodgeWindowEnded(FGameplayEventData Payload)
{
	if (Payload.OptionalObject == GetCurrentMontage())
	{
		PerfectDodgeWindowTag->EndTask();
	}
}

void UAgAbility_Dodge::HandleInvincibilityEnded(FGameplayEventData Payload)
{
	if (Payload.OptionalObject == GetCurrentMontage())
	{
		InvincibleTag->EndTask();
	}
}

void UAgAbility_Dodge::HandleRecoveryStarted(FGameplayEventData Payload)
{
	if (Payload.OptionalObject == GetCurrentMontage())
	{
		EndBusy();
	}
}

void UAgAbility_Dodge::HandlePerfectDodge(FGameplayEventData Payload)
{
	// 회피 한 번에 한 번만 받습니다. 그 뒤의 공격도 피하지만 아무것도 주지 않습니다.
	const FAgDodgeParams* Params = GetActionParams<FAgDodgeParams>();
	AAgCharacterBase* Character = GetAgCharacter();
	if (bPerfectDodged || !Params)
	{
		return;
	}
	bPerfectDodged = true;
	UE_LOG(LogAg, Verbose, TEXT("Perfect dodge: SP +%.0f, MP +%.0f, UP +%.0f"), SPSpent, Params->PerfectDodgeMPCharge, Params->PerfectDodgeUPCharge);
	const FGameplayEffectSpecHandle Reward = MakeOutgoingGameplayEffectSpec(UAgEffect_Resource::StaticClass());
	UAgEffect_Resource::SetAmounts(*Reward.Data, SPSpent, Params->PerfectDodgeMPCharge, Params->PerfectDodgeUPCharge);
	ApplyGameplayEffectSpecToOwner(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), Reward);

	// 회피 반격 기회는 이때부터 시작해 회피가 끝나도 이어집니다.
	UAgEffect_TimedTag::Apply(GetAbilitySystemComponentFromActorInfo(), AgGameplayTags::State_DodgeCounterChance, Params->CounterChanceTime);

	if (UAgTimeSubsystem* TimeSubsystem = UWorld::GetSubsystem<UAgTimeSubsystem>(Character->GetWorld()))
	{
		TimeSubsystem->StartSlowMotion(Params->SlowMotionSpeed, Params->SlowMotionDuration);
	}
}

void UAgAbility_Dodge::HandleMontageFinished()
{
	// 회피는 모션이 끝날 때 끝납니다. 달리기 전환은 후딜에서 플레이어 캐릭터가 맡습니다.
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UAgAbility_Dodge::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

bool UAgAbility_Dodge::CanPayCost(const UAgAttributeSet& Stats) const
{
	// SP가 0보다 크면 비용보다 적어도 쓸 수 있습니다(플레이어 사양 'SP 소모').
	return Stats.GetSP() > 0.f;
}

FAgAbilityCost UAgAbility_Dodge::GetCost() const
{
	const FAgDodgeParams* Params = GetActionParams<FAgDodgeParams>();
	return { Params ? Params->SPCost : 0.f, 0.f, 0.f };
}
