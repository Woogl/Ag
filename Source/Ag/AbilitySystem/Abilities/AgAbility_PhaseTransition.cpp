// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_PhaseTransition.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/AgEffect_PPRefill.h"
#include "Ag.h"
#include "Character/AgBossCharacter.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"

UAgAbility_PhaseTransition::UAgAbility_PhaseTransition()
{
	SetAssetTags(FGameplayTagContainer(AgGameplayTags::Ability_Reaction_PhaseTransition));
	ActivationOwnedTags.AddTag(AgGameplayTags::State_Invincible);

	// 보스는 패턴 실행, 이동, 패턴 후딜레이 중에만 돌고 포효하는 동안은 돌지 않습니다(보스 사양 '회전').
	ActivationOwnedTags.AddTag(AgGameplayTags::State_Boss_RotationLocked);
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Boss_Pattern);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Groggy);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_NonCombat);

	FAbilityTriggerData& Trigger = AbilityTriggers.AddDefaulted_GetRef();
	Trigger.TriggerTag = AgGameplayTags::Event_PhaseTransition;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
}

void UAgAbility_PhaseTransition::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UAnimMontage* Montage = GetCharacterData<UAgCharacterData>()->FindMontage(GetAbilityId());
	if (!Montage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogAg, Verbose, TEXT("Boss phase transition starts"));

	// 1. 포효하는 동안 무적입니다(ActivationOwnedTags).
	UAbilityTask_PlayMontageAndWait* RoarTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage);
	// 페이즈 전환은 블렌드 아웃이 시작될 때가 아니라 포효 모션이 끝날 때 끝납니다(구현 계획서 '모션 캔슬').
	RoarTask->OnCompleted.AddDynamic(this, &ThisClass::HandleRoarFinished);
	RoarTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleRoarCancelled);
	RoarTask->OnCancelled.AddDynamic(this, &ThisClass::HandleRoarCancelled);
	RoarTask->ReadyForActivation();
}

void UAgAbility_PhaseTransition::HandleRoarFinished()
{
	// 2. PP가 가득 차고, 전환이 끝나면 2페이즈가 시작됩니다. (3. AI는 기다렸다가 2페이즈 패턴을 고릅니다.)
	ApplyGameplayEffectToOwner(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), GetDefault<UAgEffect_PPRefill>(), 1.f);
	CastChecked<AAgBossCharacter>(GetAgCharacter())->EnterPhase2();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UAgAbility_PhaseTransition::HandleRoarCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
