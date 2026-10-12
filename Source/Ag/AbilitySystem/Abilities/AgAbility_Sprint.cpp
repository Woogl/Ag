// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_Sprint.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "Core/AgGameplayTags.h"

UAgAbility_Sprint::UAgAbility_Sprint()
{
	// 이동은 그대로 플레이어가 하고, 속도와 바라보는 방향만 바뀝니다.
	SetupPlayerAction(AgGameplayTags::Ability_Sprint, /*bIgnoresMovement*/ false);
	ActivationOwnedTags.AddTag(AgGameplayTags::State_Sprinting);
}

void UAgAbility_Sprint::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

bool UAgAbility_Sprint::CanPayCost(const UAgAttributeSet& Stats) const
{
	// SP가 0보다 크면 쓸 수 있습니다(플레이어 사양 'SP 소모').
	return Stats.GetSP() > 0.f;
}
