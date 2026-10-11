// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_Ultimate.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Combat/AgCombatLibrary.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"

UAgAbility_Ultimate::UAgAbility_Ultimate()
{
	SetupPlayerAction(AgGameplayTags::Ability_Action_Ultimate);
}

void UAgAbility_Ultimate::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	if (!Data || !IsActive())
	{
		return;
	}

	// 보호: 시작부터 끝까지 무적이고, 모션 캔슬로 끝나도 풀립니다.
	bInvincible = true;
	GetAbilitySystemComponentFromActorInfo()->AddLooseGameplayTag(AgGameplayTags::State_Invincible);

	// 마지막 타는 카메라 '상황별 단계'의 궁극기 마지막 일격입니다.
	FAgPlayerAttack Attack = Data->Ultimate;
	if (!Attack.Hits.IsEmpty())
	{
		Attack.Hits.Last().bFinisher = true;
	}
	StartAttack(Attack, Data->UltimateMontages);
}

void UAgAbility_Ultimate::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bInvincible)
	{
		bInvincible = false;
		GetAbilitySystemComponentFromActorInfo()->RemoveLooseGameplayTag(AgGameplayTags::State_Invincible);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UAgAbility_Ultimate::CanPayCost(const UAgAttributeSet& Stats) const
{
	// Usable only when UP is full.
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	return Data && Stats.GetMaxUP() > 0.f && Stats.GetUP() >= Stats.GetMaxUP() && Stats.GetUP() >= Data->UltimateUPCost;
}

FAgResourceAmounts UAgAbility_Ultimate::GetCost() const
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	return { 0.f, 0.f, Data ? Data->UltimateUPCost : 0.f };
}
