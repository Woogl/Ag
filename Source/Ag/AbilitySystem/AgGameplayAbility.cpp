// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "Character/AgCharacterBase.h"
#include "Core/AgGameplayTags.h"

UAgGameplayAbility::UAgGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
}

void UAgGameplayAbility::SetupPlayerAction(const FGameplayTag& AbilityTag)
{
	SetAssetTags(FGameplayTagContainer(AbilityTag));
	ActivationOwnedTags.AddTag(AgGameplayTags::State_Acting);
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Action);

	ActivationBlockedTags.AddTag(AgGameplayTags::State_Busy);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_HitReaction);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Groggy);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Execution_Executing);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Execution_Executed);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Dead);
}

AAgCharacterBase* UAgGameplayAbility::GetAgCharacter() const
{
	return Cast<AAgCharacterBase>(GetAvatarActorFromActorInfo());
}

const UAgCharacterData* UAgGameplayAbility::GetCharacterDataBase() const
{
	const AAgCharacterBase* Character = GetAgCharacter();
	return Character ? Character->GetCharacterData() : nullptr;
}

void UAgGameplayAbility::BeginBusy()
{
	if (!bBusy)
	{
		bBusy = true;
		GetAbilitySystemComponentFromActorInfo()->AddLooseGameplayTag(AgGameplayTags::State_Busy);
	}
}

void UAgGameplayAbility::EndBusy()
{
	if (bBusy)
	{
		bBusy = false;
		GetAbilitySystemComponentFromActorInfo()->RemoveLooseGameplayTag(AgGameplayTags::State_Busy);
	}
}

void UAgGameplayAbility::EndSelf(bool bWasCancelled)
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, bWasCancelled);
}

void UAgGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	EndBusy();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
