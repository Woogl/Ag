// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_Ultimate.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/AgEffect_Cost.h"
#include "AbilitySystem/Tasks/AgAbilityTask_PlayerAttack.h"
#include "Character/AgCharacterBase.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"

UAgAbility_Ultimate::UAgAbility_Ultimate()
{
	SetupPlayerAction(AgGameplayTags::Ability_Action_Ultimate);
	CostGameplayEffectClass = UAgEffect_Cost::StaticClass();

	// 보호: 시작부터 끝까지 무적이고, 모션 캔슬로 끝나도 풀립니다.
	ActivationOwnedTags.AddTag(AgGameplayTags::State_Invincible);
}

void UAgAbility_Ultimate::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	if (!Data || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 마지막 타는 카메라 '상황별 단계'의 궁극기 마지막 일격입니다.
	FAgPlayerAttack Attack = Data->Ultimate;
	if (!Attack.Hits.IsEmpty())
	{
		Attack.Hits.Last().bFinisher = true;
	}
	BeginBusy();
	UAgAbilityTask_PlayerAttack* AttackTask = UAgAbilityTask_PlayerAttack::PlayAttack(this, Attack, Data->UltimateMontages);
	AttackTask->OnLastWindowEnded.AddDynamic(this, &ThisClass::HandleLastWindowEnded);
	AttackTask->OnCompleted.AddDynamic(this, &ThisClass::HandleAttackCompleted);
	AttackTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleAttackInterrupted);
	AttackTask->ReadyForActivation();
}

void UAgAbility_Ultimate::HandleLastWindowEnded()
{
	// 마지막 판정 구간이 끝나면 후딜이 시작됩니다.
	EndBusy();
}

void UAgAbility_Ultimate::HandleAttackCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UAgAbility_Ultimate::HandleAttackInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

bool UAgAbility_Ultimate::CanPayCost(const UAgAttributeSet& Stats) const
{
	// UP가 가득 찼을 때만 쓸 수 있습니다(플레이어 사양 '궁극기').
	return Stats.GetMaxUP() > 0.f && Stats.GetUP() >= Stats.GetMaxUP();
}

FAgAbilityCost UAgAbility_Ultimate::GetCost() const
{
	// UP를 모두 씁니다.
	const AAgCharacterBase* Character = GetAgCharacter();
	return { 0.f, 0.f, Character ? Character->GetAttributeSet()->GetUP() : 0.f };
}
