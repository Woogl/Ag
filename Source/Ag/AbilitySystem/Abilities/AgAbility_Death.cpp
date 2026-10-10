// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_Death.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Character/AgCharacterBase.h"
#include "Combat/AgCombatLibrary.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "Data/AgCombatRules.h"

UAgAbility_Death::UAgAbility_Death()
{
	SetAssetTags(FGameplayTagContainer(AgGameplayTags::Ability_Reaction_Death));
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Dead);

	FAbilityTriggerData& Trigger = AbilityTriggers.AddDefaulted_GetRef();
	Trigger.TriggerTag = AgGameplayTags::Event_Death;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
}

void UAgAbility_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AAgCharacterBase* Character = GetAgCharacter();
	if (!Character)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 처형 사망: the 처형 사망 motion matches the 처형 피격 motion until after the final blow, so it continues from the
	// same position without a jump. The ragdoll comes when it ends. Only the execution's own blow does this; any other hit
	// that kills during the 처형 피격 motion is an ordinary death.
	const UAgBossData* BossData = GetCharacterData<UAgBossData>();
	const UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
	const bool bKilledByExecution = TriggerEventData && TriggerEventData->InstigatorTags.HasTagExact(AgGameplayTags::Ability_Action_Execution);
	if (bKilledByExecution && BossData && BossData->ExecutedMontage && BossData->ExecutedDeathMontage && AnimInstance
		&& GetAbilitySystemComponentFromActorInfo()->HasMatchingGameplayTag(AgGameplayTags::State_Execution_Executed))
	{
		const float Position = AnimInstance->Montage_GetPosition(BossData->ExecutedMontage);
		Character->Die(FVector::ZeroVector, this, /*bRagdoll*/ false);

		UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, BossData->ExecutedDeathMontage, 1.f, NAME_None, /*bStopWhenAbilityEnds*/ false, 1.f, Position);
		Task->OnCompleted.AddDynamic(this, &ThisClass::HandleExecutedDeathEnded);
		Task->OnBlendOut.AddDynamic(this, &ThisClass::HandleExecutedDeathEnded);
		Task->OnInterrupted.AddDynamic(this, &ThisClass::HandleExecutedDeathEnded);
		Task->OnCancelled.AddDynamic(this, &ThisClass::HandleExecutedDeathEnded);
		Task->ReadyForActivation();
		return;
	}

	// Pushed the way the last attack came from.
	const AActor* Attacker = TriggerEventData ? TriggerEventData->Instigator.Get() : nullptr;
	const UAgCombatRules* Rules = UAgCombatLibrary::GetCombatRules();
	const FVector Push = UAgCombatLibrary::GetAttackDirection(Attacker, Character) * (Rules ? Rules->DeathPushSpeed : 0.f);
	Character->Die(Push, this);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UAgAbility_Death::HandleExecutedDeathEnded()
{
	if (AAgCharacterBase* Character = GetAgCharacter())
	{
		Character->StartRagdoll(FVector::ZeroVector);
	}
	EndSelf(false);
}
