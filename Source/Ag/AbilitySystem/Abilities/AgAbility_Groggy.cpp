// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_Groggy.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/AgCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "Combat/AgCombatLibrary.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "Data/AgCombatRules.h"

namespace
{
	const FName GroggyEndSection(TEXT("End"));
}

UAgAbility_Groggy::UAgAbility_Groggy()
{
	SetAssetTags(FGameplayTagContainer(AgGameplayTags::Ability_Reaction_Groggy));
	ActivationOwnedTags.AddTag(AgGameplayTags::State_Groggy);

	// 상태 우선순위 3: stops patterns, actions and lower reactions.
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Boss_Pattern);
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Action);
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Reaction_KnockBack);
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Reaction_Down);
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Reaction_GuardPushback);
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Reaction_GuardBreak);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Groggy);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Dead);

	FAbilityTriggerData& Trigger = AbilityTriggers.AddDefaulted_GetRef();
	Trigger.TriggerTag = AgGameplayTags::Event_Groggy;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
}

void UAgAbility_Groggy::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	const UAgCombatRules* Rules = UAgCombatLibrary::GetCombatRules();
	if (!Data || !Data->GroggyMontage || !Rules || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	bExecuted = false;

	UAbilityTask_WaitGameplayEvent* ExecutedTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Event_Executed);
	ExecutedTask->EventReceived.AddDynamic(this, &ThisClass::HandleExecuted);
	ExecutedTask->ReadyForActivation();

	// The groggy time covers the whole motion, so the End section starts that long before the time is up.
	// The time is counted in game time and keeps running during hitstop (전투 시스템 '히트스톱 중의 시간').
	const UAnimMontage* Montage = Data->GroggyMontage;
	const int32 EndSection = Montage->GetSectionIndex(GroggyEndSection);
	const float EndLength = EndSection != INDEX_NONE ? Montage->GetSectionLength(EndSection) : 0.f;
	MontageTask = PlayMontage(Data->GroggyMontage);
	EndSectionTask = UAbilityTask_WaitDelay::WaitDelay(this, FMath::Max(0.f, Rules->GroggyDuration - EndLength));
	EndSectionTask->OnFinish.AddDynamic(this, &ThisClass::HandleEndSectionDue);
	EndSectionTask->ReadyForActivation();
}

UAbilityTask_PlayMontageAndWait* UAgAbility_Groggy::PlayMontage(UAnimMontage* Montage)
{
	UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage);
	Task->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageFinished);
	Task->OnBlendOut.AddDynamic(this, &ThisClass::HandleMontageFinished);
	Task->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	Task->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	Task->ReadyForActivation();
	return Task;
}

void UAgAbility_Groggy::HandleEndSectionDue()
{
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance();
	if (!bExecuted && Data && AnimInstance)
	{
		AnimInstance->Montage_JumpToSection(GroggyEndSection, Data->GroggyMontage);
	}
}

void UAgAbility_Groggy::HandleExecuted(FGameplayEventData Payload)
{
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	AAgCharacterBase* Boss = GetAgCharacter();
	if (bExecuted || !Data || !Data->ExecutedMontage || !Boss)
	{
		return;
	}
	bExecuted = true;
	GetAbilitySystemComponentFromActorInfo()->AddLooseGameplayTag(AgGameplayTags::State_Execution_Executed);

	// The two execution motions bring the characters closer than their capsules allow, so the boss stops blocking.
	Boss->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	// 처형을 시작하면 보스가 플레이어를 바라보도록 돌립니다.
	if (const AActor* Executioner = Payload.Instigator.Get())
	{
		const FVector ToExecutioner = UAgCombatLibrary::GetAttackDirection(Boss, Executioner);
		if (!ToExecutioner.IsNearlyZero())
		{
			Boss->SetActorRotation(FRotator(0.f, ToExecutioner.Rotation().Yaw, 0.f));
		}
	}

	// Groggy now ends with the 처형 피격 motion, whatever time is left.
	if (EndSectionTask)
	{
		EndSectionTask->EndTask();
		EndSectionTask = nullptr;
	}
	if (MontageTask)
	{
		MontageTask->EndTask();
	}
	MontageTask = PlayMontage(Data->ExecutedMontage);
}

void UAgAbility_Groggy::HandleMontageFinished()
{
	FinishGroggy();
}

void UAgAbility_Groggy::HandleMontageCancelled()
{
	EndSelf(true);
}

void UAgAbility_Groggy::FinishGroggy()
{
	if (AAgCharacterBase* Boss = GetAgCharacter())
	{
		const UAgAttributeSet* Stats = Boss->GetAttributeSet();
		UAgCombatLibrary::ApplyStatChange(Boss, Boss, 0.f, Stats->GetMaxPP() - Stats->GetPP());
	}
	EndSelf(false);
}

void UAgAbility_Groggy::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bExecuted)
	{
		bExecuted = false;
		GetAbilitySystemComponentFromActorInfo()->RemoveLooseGameplayTag(AgGameplayTags::State_Execution_Executed);
		AAgCharacterBase* Boss = GetAgCharacter();
		if (Boss && !Boss->IsDead())
		{
			Boss->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		}
	}
	MontageTask = nullptr;
	EndSectionTask = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
