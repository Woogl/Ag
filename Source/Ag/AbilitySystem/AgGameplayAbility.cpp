// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgGameplayAbility.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/AgEffect_PatternCooldown.h"
#include "AbilitySystem/AgEffect_TimedTag.h"
#include "AbilitySystem/Tasks/AgAbilityTask_HoldTag.h"
#include "AbilitySystemComponent.h"
#include "Character/AgBossCharacter.h"
#include "Character/AgCharacterBase.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"

namespace
{
	/**
	 * 보스 패턴 스펙의 쿨다운 출처(그 스펙의 어빌리티 인스턴스)와 쿨다운 시간(Boss.json)을 찾습니다. 보스 패턴이 아니면 null입니다.
	 * 쿨다운이 있는지는 시간(0이면 없음)이 정합니다. 클래스 기본 객체에서도 불리므로 ActorInfo와 스펙 핸들로만 찾습니다.
	 */
	const UGameplayAbility* FindPatternCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, float& OutDuration)
	{
		OutDuration = 0.f;
		const AAgBossCharacter* Boss = ActorInfo ? Cast<AAgBossCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
		const FAgBossPattern* Pattern = Boss ? Boss->FindPattern(Handle) : nullptr;
		const FGameplayAbilitySpec* Spec = Pattern ? ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle) : nullptr;
		if (!Spec)
		{
			return nullptr;
		}
		OutDuration = Pattern->Cooldown;
		return Spec->GetPrimaryInstance();
	}
}

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
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Sprint);

	ActivationBlockedTags.AddTag(AgGameplayTags::State_Busy);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_HitReaction);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Groggy);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Execution_Executing);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Execution_Executed);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Dead);
}

void UAgGameplayAbility::SetupHitReaction(const FGameplayTag& AbilityTag, const FGameplayTag& TriggerEvent)
{
	SetAssetTags(FGameplayTagContainer(AbilityTag));
	bRetriggerInstancedAbility = true;

	FAbilityTriggerData& Trigger = AbilityTriggers.AddDefaulted_GetRef();
	Trigger.TriggerTag = TriggerEvent;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;

	ActivationOwnedTags.AddTag(AgGameplayTags::State_HitReaction);
	// 같은 우선순위(상태 우선순위 4)는 새 반응이 앞 반응을 대신합니다. 행동, 달리기, 보스 패턴은 멈춥니다.
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Action);
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Sprint);
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Boss_Pattern);
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Reaction_HitReaction);

	ActivationBlockedTags.AddTag(AgGameplayTags::State_Groggy);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Execution_Executing);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Execution_Executed);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Dead);
}

void UAgGameplayAbility::SetupBossPattern()
{
	SetAssetTags(FGameplayTagContainer(AgGameplayTags::Ability_Boss_Pattern));

	ActivationBlockedTags.AddTag(AgGameplayTags::State_HitReaction);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Groggy);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Execution_Executed);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_NonCombat);
}

AAgCharacterBase* UAgGameplayAbility::GetAgCharacter() const
{
	return CastChecked<AAgCharacterBase>(GetAvatarActorFromActorInfo());
}

const UAgCharacterData* UAgGameplayAbility::GetCharacterDataBase() const
{
	return GetAgCharacter()->GetCharacterData();
}

const TInstancedStruct<FAgBossPatternParams>* UAgGameplayAbility::FindPatternParams() const
{
	const AAgBossCharacter* Boss = Cast<AAgBossCharacter>(GetAvatarActorFromActorInfo());
	const FAgBossPattern* Pattern = Boss ? Boss->FindPattern(GetCurrentAbilitySpecHandle()) : nullptr;
	return Pattern ? &Pattern->Params : nullptr;
}

void UAgGameplayAbility::BeginBusy()
{
	if (!BusyTag)
	{
		BusyTag = UAgAbilityTask_HoldTag::HoldTag(this, AgGameplayTags::State_Busy);
		BusyTag->ReadyForActivation();
	}
}

void UAgGameplayAbility::EndBusy()
{
	if (BusyTag)
	{
		BusyTag->EndTask();
		BusyTag = nullptr;
	}
}

void UAgGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 행동 중 태그는 그 태스크가 어빌리티와 함께 끝나며 뗍니다. 다음 실행이 새 태스크를 만들도록 가리키던 것만 지웁니다.
	BusyTag = nullptr;

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

bool UAgGameplayAbility::CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	float Duration = 0.f;
	if (const UGameplayAbility* PatternCooldownSource = FindPatternCooldown(Handle, ActorInfo, Duration))
	{
		return !UAgEffect_PatternCooldown::IsActive(ActorInfo->AbilitySystemComponent.Get(), PatternCooldownSource);
	}
	return Super::CheckCooldown(Handle, ActorInfo, OptionalRelevantTags);
}

void UAgGameplayAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (!ActorInfo)
	{
		return;
	}
	float PatternCooldown = 0.f;
	if (const UGameplayAbility* PatternCooldownSource = FindPatternCooldown(Handle, ActorInfo, PatternCooldown))
	{
		UAgEffect_PatternCooldown::Apply(ActorInfo->AbilitySystemComponent.Get(), PatternCooldownSource, PatternCooldown);
	}
	else if (!CooldownTags.IsEmpty())
	{
		UAgEffect_TimedTag::Apply(ActorInfo->AbilitySystemComponent.Get(), CooldownTags.First(), GetCooldownDuration());
	}
}

bool UAgGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	const AAgCharacterBase* Character = ActorInfo ? Cast<AAgCharacterBase>(ActorInfo->AvatarActor.Get()) : nullptr;
	return Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags) && Character && CanPayCost(*Character->GetAttributeSet());
}

FAgAbilityCost UAgGameplayAbility::GetCost() const
{
	return FAgAbilityCost();
}
