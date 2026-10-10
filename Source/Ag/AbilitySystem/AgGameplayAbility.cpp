// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgGameplayAbility.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Character/AgCharacterBase.h"
#include "Combat/AgCombatLibrary.h"
#include "Core/AgGameplayTags.h"

UAgGameplayAbility::UAgGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
}

void UAgGameplayAbility::SetupPlayerAction(const FGameplayTag& AbilityTag, bool bIgnoresMovement)
{
	SetAssetTags(FGameplayTagContainer(AbilityTag));
	if (bIgnoresMovement)
	{
		ActivationOwnedTags.AddTag(AgGameplayTags::State_Acting);
	}
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

void UAgGameplayAbility::SetCooldownTag(const FGameplayTag& CooldownTag)
{
	CooldownTags = FGameplayTagContainer(CooldownTag);
}

const FGameplayTagContainer* UAgGameplayAbility::GetCooldownTags() const
{
	return &CooldownTags;
}

void UAgGameplayAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (!CooldownTags.IsEmpty() && ActorInfo)
	{
		UAgCombatLibrary::ApplyTimedTag(ActorInfo->AbilitySystemComponent.Get(), CooldownTags.First(), GetCooldownDuration());
	}
}

bool UAgGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	const AAgCharacterBase* Character = ActorInfo ? Cast<AAgCharacterBase>(ActorInfo->AvatarActor.Get()) : nullptr;
	return Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags) && Character && CanPayCost(*Character->GetAttributeSet());
}

void UAgGameplayAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	Super::ApplyCost(Handle, ActorInfo, ActivationInfo);

	const FAgResourceAmounts Cost = GetCost();
	AAgCharacterBase* Character = ActorInfo ? Cast<AAgCharacterBase>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (Character && (Cost.SP > 0.f || Cost.MP > 0.f || Cost.UP > 0.f))
	{
		UAgCombatLibrary::ApplyResourceChange(Character, { -Cost.SP, -Cost.MP, -Cost.UP });
	}
}

FAgResourceAmounts UAgGameplayAbility::GetCost() const
{
	return FAgResourceAmounts();
}
