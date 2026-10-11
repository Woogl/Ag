// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_Groggy.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitMovementModeChange.h"
#include "AbilitySystem/AgEffect_PPRefill.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/AgBossCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Combat/AgCombatLibrary.h"
#include "Core/AgGameplayTags.h"
#include "Core/AgSettings.h"
#include "Data/AgCharacterData.h"
#include "Data/AgCombatRules.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	const FName GroggyEndSection(TEXT("End"));
}

UAgAbility_Groggy::UAgAbility_Groggy()
{
	SetAssetTags(FGameplayTagContainer(AgGameplayTags::Ability_Reaction_Groggy));
	ActivationOwnedTags.AddTag(AgGameplayTags::State_Groggy);

	// 상태 우선순위 3: 패턴, 행동, 달리기, 그보다 낮은 피격 반응을 멈춥니다.
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Boss_Pattern);
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Action);
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Sprint);
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Reaction_HitReaction);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Groggy);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_NonCombat);

	FAbilityTriggerData& Trigger = AbilityTriggers.AddDefaulted_GetRef();
	Trigger.TriggerTag = AgGameplayTags::Event_Groggy;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
}

void UAgAbility_Groggy::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	if (!Data || !Data->GroggyMontage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	bExecuted = false;

	UAbilityTask_WaitGameplayEvent* ExecutedTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Event_Executed);
	ExecutedTask->EventReceived.AddDynamic(this, &ThisClass::HandleExecuted);
	ExecutedTask->ReadyForActivation();

	// 공중에서 PP가 깨지면(A4) 공격은 이미 취소됐고, 그로기는 보스가 착지할 때 시작합니다.
	const AAgCharacterBase* Boss = GetAgCharacter();
	if (Boss && Boss->GetCharacterMovement()->IsFalling())
	{
		UAbilityTask_WaitMovementModeChange* LandTask = UAbilityTask_WaitMovementModeChange::CreateWaitMovementModeChange(this, MOVE_Walking);
		LandTask->OnChange.AddDynamic(this, &ThisClass::HandleLanded);
		LandTask->ReadyForActivation();
		return;
	}
	StartGroggy();
}

void UAgAbility_Groggy::HandleLanded(EMovementMode NewMovementMode)
{
	StartGroggy();
}

void UAgAbility_Groggy::StartGroggy()
{
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	if (!Data || MontageTask)
	{
		return;
	}

	// 그로기 시간은 모션 전체를 덮으므로, End 섹션은 시간이 끝나기 그 길이만큼 전에 시작합니다.
	// 시간은 게임 시간으로 세며 히트스톱 중에도 흐릅니다(전투 시스템 '히트스톱 중의 시간').
	const UAgCombatRules& Rules = UAgSettings::Get()->GetCombatRules();
	const UAnimMontage* Montage = Data->GroggyMontage;
	const int32 EndSection = Montage->GetSectionIndex(GroggyEndSection);
	const float EndLength = EndSection != INDEX_NONE ? Montage->GetSectionLength(EndSection) : 0.f;
	MontageTask = PlayMontage(Data->GroggyMontage);
	if (AAgBossCharacter* Boss = Cast<AAgBossCharacter>(GetAgCharacter()))
	{
		Boss->SetGroggyTimerRunning(true);
	}
	EndSectionTask = UAbilityTask_WaitDelay::WaitDelay(this, FMath::Max(0.f, Rules.GroggyDuration - EndLength));
	EndSectionTask->OnFinish.AddDynamic(this, &ThisClass::HandleEndSectionDue);
	EndSectionTask->ReadyForActivation();
}

UAbilityTask_PlayMontageAndWait* UAgAbility_Groggy::PlayMontage(UAnimMontage* Montage)
{
	UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage);
	// 그로기는 블렌드 아웃이 시작될 때가 아니라 모션이 끝날 때 끝납니다. 그로기 시간은 원래 자세로 돌아오는 모션의 끝까지입니다.
	Task->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageFinished);
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
	// 그로기가 끝나면 PP가 가득 찹니다.
	ApplyGameplayEffectToOwner(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), GetDefault<UAgEffect_PPRefill>(), 1.f);
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
	if (AAgBossCharacter* Boss = Cast<AAgBossCharacter>(GetAgCharacter()))
	{
		Boss->SetGroggyTimerRunning(false);
	}
	MontageTask = nullptr;
	EndSectionTask = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
