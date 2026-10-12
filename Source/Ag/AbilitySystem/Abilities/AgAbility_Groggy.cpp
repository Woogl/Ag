// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_Groggy.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitMovementModeChange.h"
#include "AbilitySystem/AgEffect_PPRefill.h"
#include "AbilitySystem/Tasks/AgAbilityTask_HoldTag.h"
#include "AbilitySystem/Tasks/AgAbilityTask_IgnorePawnCollision.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/AgBossCharacter.h"
#include "Combat/AgCombatLibrary.h"
#include "Core/AgAssetManager.h"
#include "Core/AgGameplayTags.h"
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
	if (!Data->GroggyMontage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilityTask_WaitGameplayEvent* ExecutedTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Event_Executed);
	ExecutedTask->EventReceived.AddDynamic(this, &ThisClass::HandleExecuted);
	ExecutedTask->ReadyForActivation();

	// 공중에서 PP가 0이 되면(도약 내려찍기) 공격은 이미 취소됐고, 그로기는 보스가 착지할 때 시작합니다.
	if (GetAgCharacter()->GetCharacterMovement()->IsFalling())
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
	if (MontageTask)
	{
		return;
	}

	// 그로기 시간은 모션 전체를 덮으므로, End 섹션은 시간이 끝나기 그 길이만큼 전에 시작합니다.
	// 시간은 게임 시간으로 세며 히트스톱 중에도 흐릅니다(전투 시스템 '히트스톱 중의 시간').
	const UAgCombatRules& Rules = UAgAssetManager::Get().GetCombatRules();
	const UAnimMontage* Montage = Data->GroggyMontage;
	const int32 EndSection = Montage->GetSectionIndex(GroggyEndSection);
	const float EndLength = EndSection != INDEX_NONE ? Montage->GetSectionLength(EndSection) : 0.f;
	MontageTask = PlayMontage(Data->GroggyMontage);
	CastChecked<AAgBossCharacter>(GetAgCharacter())->SetGroggyTimerRunning(true);
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
	if (AnimInstance)
	{
		AnimInstance->Montage_JumpToSection(GroggyEndSection, Data->GroggyMontage);
	}
}

void UAgAbility_Groggy::HandleExecuted(FGameplayEventData Payload)
{
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	AAgCharacterBase* Boss = GetAgCharacter();
	if (!Data->ExecutedMontage)
	{
		return;
	}
	UAgAbilityTask_HoldTag::HoldTag(this, AgGameplayTags::State_Execution_Executed)->ReadyForActivation();

	// 두 처형 모션은 캡슐 반지름보다 가깝게 붙으므로, 보스가 플레이어를 막지 않게 합니다.
	UAgAbilityTask_IgnorePawnCollision::IgnorePawnCollision(this)->ReadyForActivation();

	// 처형을 시작하면 보스가 플레이어를 바라보도록 돌립니다.
	if (const AActor* Executioner = Payload.Instigator.Get())
	{
		const FVector ToExecutioner = UAgCombatLibrary::GetHorizontalDirection(Boss, Executioner);
		if (!ToExecutioner.IsNearlyZero())
		{
			Boss->SetActorRotation(FRotator(0.f, ToExecutioner.Rotation().Yaw, 0.f));
		}
	}

	// 이제 그로기는 남은 시간과 관계없이 처형 피격 모션이 끝날 때 끝납니다.
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
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UAgAbility_Groggy::FinishGroggy()
{
	// 그로기가 끝나면 PP가 가득 찹니다.
	ApplyGameplayEffectToOwner(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), GetDefault<UAgEffect_PPRefill>(), 1.f);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UAgAbility_Groggy::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 처형 피격 태그와 캐릭터 간 충돌은 그 태스크가 어빌리티와 함께 끝나며 되돌립니다.
	CastChecked<AAgBossCharacter>(GetAgCharacter())->SetGroggyTimerRunning(false);
	MontageTask = nullptr;
	EndSectionTask = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
